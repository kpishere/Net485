# Simulation environment

Bash scripts to start services and set up networks.  

Node-red is used to wire UDP ports for a simulated RS485 network.

As components are unit-tested to be functional, they are put into service with the scripts herein.  The additional device under test can be executed from XCode in test/debug mode.

## Basic two-device network : Furnace and Thermostat

`twoUnitNetwork.sh` - is a script to start up node-red and start the socat executables.
`nodeRed_twoDevice_flow.json` - node-red flow to perform multi-plexing between ports 
 
