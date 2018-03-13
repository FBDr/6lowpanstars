# Introduction
This is a fork of the ndnSIM: NS-3 based NDN simulator repository: https://github.com/named-data-ndnSIM/ndnSIM, combined with the NS-3 repository https://github.com/nsnam/ns-3-dev-git.
The branches used for my thesis-work are the 'master' branch for the 'smart home' scenario and the 'fac3' branch for the 'smart factory' scenario.

# Structure
The simualtion set-up files can be found in the [wns-iot-v1 folder](scratch/wsn-iot-v1) folder. The implmented CoAP files are included in 
[applications folder](src/applications/model). All files start with coap-.. and are based on the [udp-client](src/applications/model/udp-client.cc)  and [udp-server](src/applications/model/udp-server.cc) apps. 

The customised ndn Zipf-Mandelbrot application can be found in the [ndn/apps folder] (src/ndnSIM/apps/ndn-consumer-zipf-mandelbrot-v2.cpp). 

# Installation
Install NS-3 according to the instructions in https://github.com/nsnam/ns-3-dev-git.
Clone this repository into the NS-3 folder.




