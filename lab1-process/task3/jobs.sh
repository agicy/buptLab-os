#!/bin/sh

tree >/dev/null &
job_id=$!
kill -STOP $job_id
jobs
kill -KILL $job_id
