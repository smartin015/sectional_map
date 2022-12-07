#!/bin/bash


gcloud storage ls gs://29f98e10-a489-4c82-ae5e-489dbcd4912f | grep "_asp.json" > asp_files.txt

mkdir asp_files

for P in $(cat ../asp_files.txt); do; gcloud storage cp $P $(basename $P) ; done

