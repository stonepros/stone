#!/usr/bin/env bash

set -ex

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
: ${STONE_ROOT:=$SCRIPTPATH/../../}

sudo docker run --rm \
         -v "$STONE_ROOT":/stone \
         --name=promtool \
         --network=host \
         dnanexus/promtool:2.9.2 \
         test rules /stone/monitoring/prometheus/alerts/test_alerts.yml
