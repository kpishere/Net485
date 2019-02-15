/*
 *  Net485Network.cpp
 *  Net485
 *
 *  Created by Kevin D Peck on 17-09-27.
 *  Copyright 2017 Shea. All rights reserved.
 *
 */

#include "Net485Network.hpp"
/*
// Purpose applies to Addr485 value shown up to next 
#define ADDR485ASSIGNMENT_RANGES 11
#define ORDEREDLIST_COLUMNS 2 
const unsigned short Addr485AssignmentOfPurpose[ADDR485ASSIGNMENT_RANGES][ORDEREDLIST_COLUMNS] = {
	{0x00, Address485Purpose.Broadcast},
	{0x01, Address485Purpose.Subnet_2_Subordinate},
	{0x0F, Address485Purpose.Reserved}, // Overhead validation
	{0x10, Address485Purpose.Subnet_3_Subordinate},
	{0x3F, Address485Purpose.Reserved}, // Overhead validation, Future Internet access
	{0x55, Address485Purpose.Restricted}, // Diagnostic use
	{0x5B, Address485Purpose.Reserved}, // Future Wireless
	{0xC0, Address485Purpose.Restricted}, // Network Analysis
	{0xC1, Address485Purpose.Reserved}, // Future system Authentication
	{0xFE, Address485Purpose.Arbitration},
	{0xFF, Address485Purpose.Coordinator}
}
#define SUBNET485ASSIGNMENT_RANGES 3
const unsigned short SubNet485AssignmentOfPurpose[ADDR485ASSIGNMENT_RANGES][ORDEREDLIST_COLUMNS] = {
	{0x00, SubNet485Purpose.Broadcast},
	{0x01, SubNet485Purpose.Reserved},
	{0xFF, SubNet485Purpose.Reserved}
}
*/

Net485Network::Net485Network(Net485DataLink *_net, bool _coordinatorCapable) {
    this->net485dl = _net;
    this->sessionId = this->net485dl->newSessionId();
    this->coordinatorCapable = _coordinatorCapable;
}
