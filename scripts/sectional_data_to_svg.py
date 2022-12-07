import sys
import math
import json
import svgwrite

# The airspace DB lists locations by longitude (X-axis) then latitude (Y-axis)
BOUNDS_LONLAT = [
    [-87.807856, 31.091132],
    [-79.224909, 24.979881],
]
SHAPES_PER_ROW = 10

def read_data():
    result = []
    for path in sys.argv[1:]:
        with open(path, 'r') as f:
            result += json.loads(f.read())
    print(f"Loaded {len(result)} results from {len(sys.argv[1:])} files")
    return result

def compute_bounds(poly):
    gmin = [float('inf'), float('inf')]
    gmax = [float('-inf'), float('-inf')]
    for pt in poly:
        gmin = [min(a,b) for (a,b) in zip(gmin, pt)]
        gmax = [max(a,b) for (a,b) in zip(gmax, pt)]
    return [(gmin[0], gmax[1]), (gmax[0], gmin[1])]

def filter_by_bounds(bounds, data):
    def valid_and_in_bounds(d):
        if d.get("geometry") is None or d["geometry"].get("coordinates") is None:
            return False
        gmin = [float('inf'), float('inf')]
        gmax = [float('-inf'), float('-inf')]
        for poly in d["geometry"]["coordinates"]:
            b = compute_bounds(poly)
            in_bounds = (
                    b[0][0] > bounds[0][0] and 
                    b[1][1] > bounds[1][1] and
                    b[1][0] < bounds[1][0] and 
                    b[0][1] < bounds[0][1])
            if not in_bounds:
                return False
        return True
    return [d for d in data if valid_and_in_bounds(d)]

def filter_by_airspace_classes(data):
    # Filter down only to airspaces of class A, C, D, and E
    # and exclude 'special' type airspaces
    # "name":"ORLANDO CLASS B AREA A","type":0,"icaoClass":1
    # "name":"ORLANDO CLASS D","type":0,"icaoClass":3
    return [d for d in data if 
            d["icaoClass"] in (0, 2, 3, 4) and
            d["type"] == 0
    ]


def dist_between(lon1, lat1, lon2, lat2):
    # From https://www.movable-type.co.uk/scripts/latlong.html
    # haversine formula
    R = 6371e3 # metres
    PI=3.14159265
    phi1 = lat1 * PI/180
    phi2 = lat2 * PI/180
    dphi = (lat2-lat1) * PI/180
    dlambda = (lon2-lon1) * PI/180
    # print(phi1, phi2, dphi, dlambda)
    a = (
        math.sin(dphi/2) * math.sin(dphi/2) +
        math.cos(phi1) * math.cos(phi2) *
        math.sin(dlambda/2) * math.sin(dlambda/2)
    )
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1-a))
    return R * c # in metres

# print("dist_between check:", dist_between(-79.980157, 40.329309, -79.879918, 40.327270), " <-- should be 5.37 mi (8.64 km)")
# sys.exit(0)

FACTOR = 764.978

M_PER_NMILE = 1852
SCALE_NMILE_PER_MM = 27 
def scale_lonlat(poly):
    b = compute_bounds(poly)
    diag_dist_m = dist_between(b[0][0], b[0][1], b[1][0], b[1][1])
    # print("scaling by", factor)
    poly = normalize(poly)
    poly = [(pt[0]*FACTOR, pt[1]*FACTOR) for pt in poly]
    return layout(poly)

def normalize(poly):
    b = compute_bounds(poly)
    # print("Bounds", b)
    center = [
            (b[1][0] - b[0][0])/2 + b[0][0],
            (b[0][1] - b[1][1])/2 + b[1][1]
            ]
    # print("Normalizing to center", center, "e.g.", poly[0])
    return [ (pt[0]-center[0], pt[1]-center[1]) for pt in poly ]

def layout(poly):
    b = compute_bounds(poly)
    # print("shifting:", b[0])
    return [ (pt[0]-b[0][0], pt[1]-b[1][1]) for pt in poly ]

ACRYLIC_THICKNESS_IN = 0.25
INCH_PER_FT = 1.0/4000
def make_pairs(data):
    geotypes = set([a['geometry']['type'] for a in data])
    assert list(geotypes) == ['Polygon']

    result = []
    for a in data:
        thickness = (a['upperLimit']['value'] - a['lowerLimit']['value']) * INCH_PER_FT
        repeats = int(thickness / ACRYLIC_THICKNESS_IN)
        print(repeats, "repeats for thickness", thickness)
        for g in a['geometry']['coordinates']:
            g2 = scale_lonlat(g)
            for i in range(repeats):
                result.append((a['name'], g2))
    return result


SPACING = 50
def save_as_svg(poly_id_pairs, outpath):
    dwg = svgwrite.Drawing(outpath, profile='tiny', size=("100mm", "100mm"))
    for i, pp in enumerate(poly_id_pairs):
        n, poly = pp
        row = int(i / SHAPES_PER_ROW)
        col = int(i % SHAPES_PER_ROW)
        
        grp = svgwrite.container.Group()
        grp.add(dwg.polygon(points=poly, stroke='black', fill="none"))
        grp.add(dwg.text(str(n), fill='red', x=('50',), y=('50',)))
        dwg.add(grp)

    dwg.save()


if __name__ == "__main__":
    data = read_data()
    data = filter_by_bounds(BOUNDS_LONLAT, data)
    print(f"Filtered down to {len(data)} results within bounds {BOUNDS_LONLAT}")
    data = filter_by_airspace_classes(data)
    print(f"Filtered down to {len(data)} results by restricting ICAO class and type")

    pairs = make_pairs(data)
    pairs.sort(key=lambda pp: pp[0])
    STRIDE = 10
    for i in range(0, len(pairs), STRIDE):
        save_as_svg(pairs[i:i+STRIDE], f"out/{i:03d}_to_{i+STRIDE-1:03d}.svg")
        
    # print(pairs[0])
