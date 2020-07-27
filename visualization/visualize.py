"""
=====================================
Anonymous communication visualization
=====================================

"""
__author__      = "Jan B."

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import matplotlib
from threading import Timer
from threading import Thread
import random
import math
from http.server import HTTPServer, BaseHTTPRequestHandler
import ujson
import datetime

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-t", "--roundlen", type=int, default=7000, help="Length of a round in milliseconds")
parser.add_argument("--dpi", type=int, default=60, help="dpi of animation")
parser.add_argument("--save", type=int, default=0, help="Number of seconds of the animation to save to a file (if 0, will show animation live instead)")
parser.add_argument("-d", "--directory", default="dumps", help="Directory where to read the roundi.txt files (if save flag is given)")
parser.add_argument("--skip", default=0, type=int, help="First round to start with (if save flag is given)")
parser.add_argument("--noblit", action='store_true', help="Disables blitting, resulting in worse performance, but may circumvent some animation issues")
parser.set_defaults(noblit=False)
args = parser.parse_args()


#Settings
dpi = args.dpi
physicalRoutingNodeSize = 200 #in dots
overlayNodeSize = 1 #radius in distance on axis
roundlen = args.roundlen #in milliseconds
saveAnimation = True if args.save > 0 else False #if False, will show animation live. If True, will use data from the dumps directory and render those as a file
saveduration = args.save
savedir = args.directory
currentRound = args.skip-1 #(used if saveAnimation is true)
useBlitting = not args.noblit

if saveAnimation:
    import os
    import requests
else:
    matplotlib.use("TkAgg")

def getCurrentTime():
    if not saveAnimation: #time works as you'd think. With a real clock.
        return int(datetime.datetime.now().timestamp() * 1000) #milliseconds since epoch
    else: #we fake time using the frame counter
        return int(currentFrame/fpms)

# Profiler helpers
start = datetime.datetime.now()
def start_profile():
    global start
    start = datetime.datetime.now()

def stop_profile():
    end = datetime.datetime.now()
    global start
    print(end-start)

#
# Define http server
#
class AnonCommHTTPRequestHandler(BaseHTTPRequestHandler):        
    def do_POST(self):
        global dataForRound, lastFullRound
        
        #Headers
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()

        #Read data
        self.data_string = self.rfile.read(int(self.headers['Content-Length']))
        try:
            data = ujson.loads(self.data_string)
        except:
            print("Error decoding")
            return

        #Run update animation
        updateState(data)
        
        #Answer
        self.wfile.write(bytes("ok", 'utf-8'))

#
# Start Server
#
def runServer():
    server_address = ('', 8081)
    httpd = HTTPServer(server_address, AnonCommHTTPRequestHandler)
    httpd.serve_forever()

serverThread = Thread(target=runServer)
serverThread.setDaemon(True)
serverThread.start()

previousData = dict()
haveNodesMovedLastRound = False
def updateState(data): #data: dict (int physNodeId -> whatever json they sent for this round)
    global haveNodesMovedLastRound
    global previousData #to make sure we only animate changes
    global currentPhases #to update phases for animation
    t = getCurrentTime()
    print("running update")
    if len([phase for phase in currentPhases if phase._last_time > t]) > 0:
        print("Warning: previous animation did not finish. Aborting that animation and starting the next one.")
    
    ### START DEBUG STUFF
    nodes_to_onid_emul = {nodeid:data[nodeid]['onid_emul'] for nodeid in data}
    onid_emul_to_nodes = dict()
    for onid in range(8):
        onid_emul_to_nodes[str(onid)] = set([nodeid for nodeid in data if data[nodeid]['onid_emul'] == onid])
    
    for onid in range(8):
        print(str(onid) + " emulated by " + ",".join(list(onid_emul_to_nodes[str(onid)])))

    for nodeid in sorted([int(x) for x in data.keys()]):
        #Gather data
        nodedataid = str(nodeid) #(string for accessing the data we get from json)
        nodeid = int(nodeid)
        nodedata = data[nodedataid]
        
        #Check completeness of routing neighborhoods
        for onid in nodedata['gamma_route']:
            missing = [i for i in onid_emul_to_nodes[onid] if i not in nodedata['gamma_route'][onid]]
            if missing:
                #print(str(nodeid) + "'s " + str(onid) + "-quorum is missing "+str(missing))
                pass
        
        #Check correctness of routing neighborhoods
        for onid in nodedata['gamma_route']:
            missing = [i for i in nodedata['gamma_route'][onid] if i not in onid_emul_to_nodes[onid]]
            if missing:
                #print(str(nodeid) + "'s " + str(onid) + "-quorum lists "+str(missing)+", which do not feel responsible for "+str(onid))
                pass
        
        #Check superfluous routing neighborhoods
        for onid in nodedata['gamma_route']:
            onidRcvStr = overlayNodesNumeric[int(onid)]._onid
            onidSndStr = overlayNodesNumeric[int(nodedata['onid_emul'])]._onid
            hammingdistance = len([i for i in range(len(onidRcvStr)) if onidRcvStr[i] != onidSndStr[i]])
            if hammingdistance > 1:
                #print(str(nodeid)+ " (emulating "+ onidSndStr +") has non-empty neighborhood for "+onidRcvStr+": "+str(nodedata['gamma_route'][onid]))
                pass

    ### END DEBUG STUFF
      
    #Phases
    round = [data[nodedata]['cur_round'] for nodedata in data][0]
    firstPhase = math.ceil(t)
    phaselen = math.floor((roundlen) / 10) #some time reserved for nodes to switch places at the beginning
    timeWhereNodesMove = firstPhase
    phaseWhereNodeAnnounces = Phase(firstPhase + 1*phaselen, firstPhase + 1*phaselen + phaselen*15/16, dummyColor=(52/255, 117/255, 0/255, .8), name="Announcement", roundnum=round)
    phaseWhereNodeAgrees = Phase(firstPhase + 2*phaselen, firstPhase + 2*phaselen + phaselen*15/16, dummyColor=(78/255, 173/255, 0/255, .8), name="Agreement", roundnum=round)
    phaseWhereNodeInjects = Phase(firstPhase + 3*phaselen, firstPhase + 3*phaselen + phaselen*15/16, dummyColor=(94/255, 209/255, 0/255, .8), name="Injection", roundnum=round)
    phaseWhereNodeRoutes = Phase(firstPhase + 4*phaselen, firstPhase + 5*phaselen + phaselen*15/16, dummyColor=(144/255, 0/255, 163/255, .8), name="Routing", roundnum=round) #routing gets two time slots
    phaseWhereNodePredelivers = Phase(firstPhase + 6*phaselen, firstPhase + 6*phaselen + phaselen*15/16, dummyColor=(0/255, 127/255, 125/255, .8), name="Pre-delivery", roundnum=round)
    phaseWhereNodeDelivers = Phase(firstPhase + 7*phaselen, firstPhase + 7*phaselen + phaselen*15/16, dummyColor=(0/255, 204/255, 200/255, .8), name="Delivery", roundnum=round)
    phaseWhereNodeReconfigures = Phase(firstPhase + 8*phaselen, firstPhase + 9*phaselen + phaselen*15/16, dummyColor=(179/255, 150/255, 0/255, .8), name="Reconfiguration", roundnum=round)
    
    
    timeWhereNodeReceivesFirstNoneDummyMessage = dict({int(n):math.inf for n in data})


    #Move routing nodes (before main loop so that messages sent know the new location if necessary).
    haveRoutingNodesMoved = False
    global nextRoutingNodeLocations
    someNodeIsInRoundZero = False
    for nodeid in data:
        #Gather data
        nodedataid = nodeid #(string for accessing the data we get from json)
        nodeid = int(nodeid)
        nodedata = data[nodedataid]
        routingNode = physicalRoutingNodes[nodeid]
        sendReceiveNode = physicalSendReceiveNodes[nodeid]

        #Move routing nodes
        if nodedataid not in previousData or previousData[nodedataid]['onid_emul'] != nodedata['onid_emul']:
            x,y = routingNode.changeOverlayNode(overlayNodesNumeric[nodedata['onid_emul']])
            nextRoutingNodeLocations[nodeid] = [x,y]
            haveRoutingNodesMoved = True
        
        #Unmark send-receive-node if it does not hold a received msg
        if not (nodedata['has_delivered_ready_message'] or nodedata['has_delivered_unready_message']):
            sendReceiveNode.loseReceivedMsg() #will be overwritten below if the node receives some message this round. 

    
    if haveRoutingNodesMoved:
        if someNodeIsInRoundZero or not previousData:
            moveRoutingNodes(t, t+1) #move immediately on round 0 or on first animated round.
        else:
            moveRoutingNodes(timeWhereNodesMove, timeWhereNodesMove + phaselen)
    
    #Main loop (messages)
    for nodeid in data:
        #Gather data
        nodedataid = nodeid #(string for accessing the data we get from json)
        nodeid = int(nodeid)
        nodedata = data[nodedataid]
        
        routingNode = physicalRoutingNodes[nodeid]
        sendReceiveNode = physicalSendReceiveNodes[nodeid]

        timeWhereNodeSendsLastNoneDummyMessage = 0
        
        if nodedata['cur_round'] == 0:
            someNodeIsInRoundZero = True

        #If the node currently has a received pending message, mark it red at beginning of round
        if nodedata['has_delivered_ready_message'] or nodedata['has_delivered_unready_message']:
            sendReceiveNode.gainReceivedMsg(0)
            
        #Send announcements and remove sending message marker
        if 'out_announce' in nodedata:
            for receiver in nodedata['out_announce']:
                isDummy = (next((x for x in nodedata['out_announce'][receiver] if x['m'] != "DUMMY"), None) is None)
                arrivaltime = sendAnnouncement(sendReceiveNode, getPhysRoutingNode(receiver), phaseWhereNodeAnnounces, isDummy=isDummy)
                sendReceiveNode.loseBufferedMsg(phaseWhereNodeAnnounces._first_time) #if after this round, the node still has buffered send msgs, the marker will be re-set after the outer for loop.
                if not isDummy:
                    timeWhereNodeReceivesFirstNoneDummyMessage[int(receiver)] = min(timeWhereNodeReceivesFirstNoneDummyMessage[int(receiver)], arrivaltime)
                
        #Send agreement
        if 'out_agreement' in nodedata:
            if 'out_agreement' in nodedata:
                for receiver in nodedata['out_agreement']:
                    isDummy = (next((x for x in nodedata['out_agreement'][receiver] if x['m'] != "DUMMY"), None) is None)
                    arrivaltime = sendAgreement(routingNode, getPhysRoutingNode(receiver), phaseWhereNodeAgrees, isDummy=isDummy)
                    if not isDummy:
                        timeWhereNodeSendsLastNoneDummyMessage = max(timeWhereNodeSendsLastNoneDummyMessage, phaseWhereNodeAgrees._first_time)
                        timeWhereNodeReceivesFirstNoneDummyMessage[int(receiver)] = min(timeWhereNodeReceivesFirstNoneDummyMessage[int(receiver)], arrivaltime)

        #Send injection
        if 'out_inject' in nodedata:
            for receiver in nodedata['out_inject']:
                isDummy = (next((x for x in nodedata['out_inject'][receiver] if x != "DUMMY"), None) is None)
                arrivaltime = sendInject(routingNode, getPhysRoutingNode(receiver), phaseWhereNodeInjects, isDummy=isDummy)
                if not isDummy:
                    timeWhereNodeSendsLastNoneDummyMessage = max(timeWhereNodeSendsLastNoneDummyMessage, phaseWhereNodeInjects._first_time)
                    timeWhereNodeReceivesFirstNoneDummyMessage[int(receiver)] = min(timeWhereNodeReceivesFirstNoneDummyMessage[int(receiver)], arrivaltime)
        
        #Send routing
        if 'out_routing' in nodedata:
            for receiver in nodedata['out_routing']:
                
                onidSend = nodedata['onid_emul']
                onidReceive = data[str(receiver)]['onid_emul'] if str(receiver) in data else "?" # getPhysRoutingNode(receiver)._overlayNode._onid)
                onidSend = str(overlayNodesNumeric[onidSend]._onid)
                onidReceive = "???" if onidReceive == "?" else str(overlayNodesNumeric[onidReceive]._onid)
                hammingdistance = len([i for i in range(len(onidSend)) if onidSend[i] != onidReceive[i]])
                
                isDummy = (next((x for x in nodedata['out_routing'][receiver] if x != "RDUM"), None) is None)
                if hammingdistance <= 1 or haveNodesMovedLastRound or not isDummy: #TODO remove this by making the implementation stop sending dummys where none are needed (although it's consistent with the paper right now)
                    arrivaltime = sendRouting(routingNode, getPhysRoutingNode(receiver), phaseWhereNodeRoutes, isDummy=isDummy)
                    if not isDummy:
                        timeWhereNodeSendsLastNoneDummyMessage = max(timeWhereNodeSendsLastNoneDummyMessage, phaseWhereNodeRoutes._first_time)
                        timeWhereNodeReceivesFirstNoneDummyMessage[int(receiver)] = min(timeWhereNodeReceivesFirstNoneDummyMessage[int(receiver)], arrivaltime)
                    
        if 'out_routing' not in nodedata:
            print(str(nodeid)+" has sent empty routing data")
           
        
        #Send predeliver
        if 'out_predeliver' in nodedata:
            for receiver in nodedata['out_predeliver']:
                isDummy = (next((x for x in nodedata['out_predeliver'][receiver] if x != "DUMMY"), None) is None)
                arrivaltime = sendPredeliver(routingNode, getPhysRoutingNode(receiver), phaseWhereNodePredelivers, isDummy=isDummy)
                if not isDummy:
                    timeWhereNodeSendsLastNoneDummyMessage = max(timeWhereNodeSendsLastNoneDummyMessage, phaseWhereNodePredelivers._first_time)
                    timeWhereNodeReceivesFirstNoneDummyMessage[int(receiver)] = min(timeWhereNodeReceivesFirstNoneDummyMessage[int(receiver)], arrivaltime)

        #Send deliver
        if 'out_deliver' in nodedata:
            for receiver in nodedata['out_deliver']:
                isDummy = (next((x for x in nodedata['out_deliver'][receiver] if x != "DUMMY"), None) is None)
                arrivaltime = sendDeliver(routingNode, getPhysSendReceiveNode(receiver), phaseWhereNodeDelivers, isDummy=isDummy)
                if not isDummy:
                    timeWhereNodeSendsLastNoneDummyMessage = max(timeWhereNodeSendsLastNoneDummyMessage, phaseWhereNodeDelivers._first_time)
                    getPhysSendReceiveNode(receiver).gainReceivedMsg(arrivaltime)
        
        #Send reconfiguration
        if 's_overlay_prime' in nodedata:
            for receiver in nodedata['s_overlay_prime']:
                sendReconf(routingNode, getPhysRoutingNode(receiver), phaseWhereNodeReconfigures)
        
        #Make sure node will be unmarked when it sends its last message
        routingNode.defaultUnmarkAt(timeWhereNodeSendsLastNoneDummyMessage)
    
    
    #Loop to overwrite stuff inferred in main loop with data (e.g., noting that after this round, there are still msgs to send in some node's buffer)
    for nodeid in data:
        #Gather data
        nodedataid = nodeid #(string for accessing the data we get from json)
        nodeid = int(nodeid)
        nodedata = data[nodedataid]
        routingNode = physicalRoutingNodes[nodeid]
        sendReceiveNode = physicalSendReceiveNodes[nodeid]
        
        #Mark node if it's still (after this round) buffering a sending msg
        if nodedata['has_waiting_message']:
            sendReceiveNode.setBufferedMsg()

    #Mark routing nodes that have received non-dummy data
    for receiver in timeWhereNodeReceivesFirstNoneDummyMessage:
        getPhysRoutingNode(receiver).remarkAt(timeWhereNodeReceivesFirstNoneDummyMessage[receiver])
    
    
    #Finalize
    previousData = data
    haveNodesMovedLastRound = haveRoutingNodesMoved
    phases = [phaseWhereNodeAnnounces, phaseWhereNodeAgrees, phaseWhereNodeInjects, phaseWhereNodeRoutes, phaseWhereNodePredelivers, phaseWhereNodeDelivers, phaseWhereNodeReconfigures]
    for phase in phases:
        phase.finalize()
    currentPhases = phases

def getPhysRoutingNode(nodeid):
    nodeid = int(nodeid)
    return physicalRoutingNodes[nodeid]

def getPhysSendReceiveNode(nodeid):
    nodeid = int(nodeid)
    return physicalSendReceiveNodes[nodeid]

def sendMessage(sender, receiver, phase, isDummy=True): #returns time where msg will arrive
    method = phase.addDummy if isDummy else phase.addMsg
    method(sender.getCurrentXY(phase._first_time), receiver.getCurrentXY(phase._last_time))
    return phase._last_time

def sendAnnouncement(sender, receiver, phase, isDummy=True):
    if not isDummy:
        print("Sending announcement", sender._overlayNode._onid, receiver._overlayNode._onid)
        pass
    return sendMessage(sender, receiver, phase, isDummy)

def sendAgreement(sender, receiver, phase, isDummy=True):
    if not isDummy:
        print("Sending agreement", sender._overlayNode._onid, receiver._overlayNode._onid)
        pass
    return sendMessage(sender, receiver, phase, isDummy)

def sendInject(sender, receiver, phase, isDummy=True):
    if not isDummy:
        print("Sending inject", sender._overlayNode._onid, receiver._overlayNode._onid)
        pass
    return sendMessage(sender, receiver, phase, isDummy)

def sendRouting(sender, receiver, phase, isDummy=True):
    if not isDummy:
        print("Sending routing", sender._overlayNode._onid, receiver._overlayNode._onid)
        pass
    return sendMessage(sender, receiver, phase, isDummy)

def sendPredeliver(sender, receiver, phase, isDummy=True):
    if not isDummy:
        print("Sending predeliver", sender._overlayNode._onid, receiver._overlayNode._onid)
        pass
    return sendMessage(sender, receiver, phase, isDummy)

def sendDeliver(sender, receiver, phase, isDummy=True):
    if not isDummy:
        print("Sending deliver", sender._overlayNode._onid, receiver._overlayNode._onid)
        pass
    return sendMessage(sender, receiver, phase, isDummy)

def sendReconf(sender, receiver, phase, isDummy=True):
    return sendMessage(sender, receiver, phase, isDummy)


class PositionedObject:
    def __init__(self, x, y, r=0,g=0,b=0,alpha=1):
        self._x = x #desired position
        self._y = y
        self._r = r
        self._g = g
        self._b = b
        self._alpha = alpha

    def getCurrentXY(self, time):
        return [self._x, self._y]

    def getColor(self, time):
        return (self._r,self._g,self._b,self._alpha)

    def setColor(self, r,g,b,alpha):
        self._r = r
        self._g = g
        self._b = b
        self._alpha = alpha

    def getSize(self, time):
        return 100

class PhysicalRoutingNode(PositionedObject):
    def __init__(self, overlayNode, name):
        self._name = name
        self._overlayNode = overlayNode
        self._timeMarkStart = math.inf 
        self._timeMarkEnd = math.inf #node will be marked in times between start and end, and
        self._timeMarkFromThisPointOn = math.inf #node will be marked after this point until infinity. Node will not be marked in any other times.
        x,y = overlayNode.join(self)
        super().__init__(x,y)
        self.resetColorToDefault()
        
    def defaultUnmarkAt(self, time):
        self._timeMarkStart = self._timeMarkFromThisPointOn
        self._timeMarkEnd = time
        self._timeMarkFromThisPointOn = math.inf
    
    def remarkAt(self, time):
        self._timeMarkFromThisPointOn = time
    
    def isMarked(self, time):
        return self._timeMarkFromThisPointOn <= time or self._timeMarkStart <= time and self._timeMarkEnd > time
    
    def getSize(self, time):
        return physicalRoutingNodeSize
        
    def getCurrentXY(self, time):
        if time == 0:
            return [self._x, self._y]
        return getLocationOfRoutingNode(self._name, time)

    def getOverlayNode(self):
        return self._overlayNode

    def changeOverlayNode(self, nextOverlayNode): #unregisters from old overlay node and moves to new overlay node. Return x and y of new location
        self._overlayNode.leave(self)
        self._overlayNode = nextOverlayNode
        x,y  = nextOverlayNode.join(self)
        self._x = x
        self._y = y
        return x,y
        
    def getColor(self, time):
        return (1,0,0,1) if self.isMarked(time) else (self._r, self._g, self._b, self._alpha)

    def resetColorToDefault(self):
        self.setColor(16/255,112/255,181/255,1)
        
class PhysicalSendReceiveNode(PositionedObject):
    def __init__(self, overlayNode, name):
        self._name = name
        self._overlayNode = overlayNode
        self._timeLoseBufferedMsg = 0 #first time where node stops indicating buffering a send message (may be math.inf)
        self._timeGainReceivedMsg = math.inf #first time where node starts indicating buffering a received message (may be math.inf)
        x, y = overlayNode.addSendReceiveNode(self)
        super().__init__(x,y)
        self.resetColorToDefault()
    
    def setBufferedMsg(self): #marks this node to have a buffered msg for the foreseeable future (call after loseBufferedMsg())
        self._timeLoseBufferedMsg = math.inf
    
    def loseBufferedMsg(self, time): #marks this node losing a buffered msg at the latest in given time
        self._timeLoseBufferedMsg = min(self._timeLoseBufferedMsg, time)
    
    def gainReceivedMsg(self, time): #marks this node to have a received msg at the latest in given time
        self._timeGainReceivedMsg = min(self._timeGainReceivedMsg, time)
        
    def loseReceivedMsg(self): #marks this node to have no received messages
        self._timeGainReceivedMsg = math.inf
    
    def getSize(self, time):
        return physicalRoutingNodeSize

    def getOverlayNode(self):
        return self._overlayNode

    def getColor(self, time):
        if self._timeLoseBufferedMsg > time:
            return (1, 0, 0, 1)
        if self._timeGainReceivedMsg < time:
            return (183/255, 23/255, 23/255, 1)
        return (self._r,self._g,self._b,self._alpha)
        
    def resetColorToDefault(self):
        self.setColor(r=130/255, g=196/255, b=244/255, alpha=1)


class OverlayNode:
    def __init__(self, onid, x, y):
        self._x = x
        self._y = y
        self._onid = onid
        self._content = set()
        self._associated = set()

    def setNeighbors(self, neighbors): #neighbors: list of OverlayNodes
        self._neighbors = neighbors

    def getSize(self, time):
        return overlayNodeSize

    def getOnid(self):
        return self._onid

    def getNeighbors(self):
        return self._neighbors

    def getPhysicalNodes(self):
        return self._content

    def join(self, physNode): #returns coordinates for the physical node to go to and stores the physical node
        self._content.add(physNode)
        x,y = None, None
        while x is None or y is None or math.sqrt((x-self._x)**2 + (y-self._y)**2) > overlayNodeSize: #TODO this can be done nicer
            x,y = self._x+random.uniform(-overlayNodeSize, overlayNodeSize), self._y+random.uniform(-overlayNodeSize, overlayNodeSize)
        return x,y

    def leave(self, physNode):
        self._content.remove(physNode)

    def getPhysicalSendReceiveNodes(self):
        return self._associated

    def addSendReceiveNode(self, node): #returns coordinates for the node to go
        x,y = None, None
        while x is None or math.sqrt((x-self._x)**2 + (y-self._y)**2) < overlayNodeSize*1.3 or len(self._associated) > 0 and min([1000 if n == node else (x-n._x)**2 + (y-n._y)**2 for n in self._associated]) < .5:
            x,y = self._x+random.uniform(-overlayNodeSize*1.8, overlayNodeSize*1.8), self._y+random.uniform(-overlayNodeSize*1.8, overlayNodeSize*1.8)
        self._associated.add(node)
        #print(min([1000 if n == node else (x-n._x)**2 + (y-n._y)**2 for n in self._associated]))
        return x,y


def drawOverlayNode(node, plt):
    circle = plt.Circle((node._x, node._y), overlayNodeSize, facecolor=(1,1,1,1), zorder=10, edgecolor=(16/255,112/255,181/255,1), )
    axstatic.add_artist(circle)

def drawOverlayNodes(overlayNodes, plt):
    for node in overlayNodes:
        drawOverlayNode(node, plt)
        for neighbor in node.getNeighbors():
            if neighbor.getOnid() < node.getOnid():
                axstatic.plot([neighbor._x, node._x], [neighbor._y, node._y], 'k-', lw=2, zorder=5)


# Create new Figure and an Axes which fills it.
fig = plt.figure(figsize=(15, 15), dpi=dpi)
fig.canvas.set_window_title('Provably Secure Anonymous Communication with Trusted Execution Environments')
#fig, (ax1, ax2) = plt.subplots(1, 2, sharex=True,sharey=True,dpi=dpi)
axstatic = fig.add_axes([0, 0, 1, 1], frameon=False, label="static")
axstatic.set_xlim(0, 16), axstatic.set_xticks([])
axstatic.set_ylim(0, 16), axstatic.set_yticks([])
ax = fig.add_axes([0, 0, 1, 1], frameon=False, label="msgs") #axe for messages and such
ax.set_xlim(0, 16), ax.set_xticks([])
ax.set_ylim(0, 16), ax.set_yticks([])

#Set up overlay network
overlayNodes = {"000":OverlayNode("000", 10, 10),
                "001":OverlayNode("001", 10, 2),
                "010":OverlayNode("010", 2, 10),
                "011":OverlayNode("011", 2, 2),
                "100":OverlayNode("100", 14, 14),
                "101":OverlayNode("101", 14, 6),
                "110":OverlayNode("110", 6, 14),
                "111":OverlayNode("111", 6, 6),
                }

#Index overlay nodes by number
overlayNodesNumeric = {int(x, 2): y for x,y in overlayNodes.items()}

#Set up neighborhoods
for name in overlayNodes:
    neighbors = [overlayNodes[name[:i]+("1" if name[i] == "0" else "0")+name[i+1:]] for i in range(len(name))]
    overlayNodes[name].setNeighbors(neighbors)

#Draw
drawOverlayNodes(overlayNodes.values(), plt)

numPhysNodes = 81
#Set up physical send/receive nodes
physicalSendReceiveNodes = list()
for i in range(numPhysNodes):
    physicalSendReceiveNodes += [PhysicalSendReceiveNode(list(overlayNodes.values())[min(7, i//10)], i)] #TODO don't hardcode this.

#Write node names
for node in physicalSendReceiveNodes:
    x,y = node.getCurrentXY(0)
    axstatic.text(x+.1,y, node._name, size=12, zorder=21, color="black")
    

#Draw connection between send/receive node and overlay nodes
for node in physicalSendReceiveNodes:
    axstatic.plot([node._x, node.getOverlayNode()._x], [node._y, node.getOverlayNode()._y], 'k-', lw=1, zorder=5)

#Set up physical routing nodes
physicalRoutingNodes = list()
for i in range (numPhysNodes):
    physicalRoutingNodes += [PhysicalRoutingNode(list(overlayNodes.values())[i%len(overlayNodes)], i)]

# Construct the scatter which we will update during animation
physNodeScat = ax.scatter([o.getCurrentXY(0)[0] for o in physicalRoutingNodes], [o.getCurrentXY(0)[1] for o in physicalRoutingNodes],
                  zorder=25, marker="o", s=100)
physSendReceiveNodeScat = ax.scatter([o.getCurrentXY(0)[0] for o in physicalSendReceiveNodes], [o.getCurrentXY(0)[1] for o in physicalSendReceiveNodes],
                  zorder=20, marker="o", s=[o.getSize(0) for o in physicalSendReceiveNodes], c=[o.getColor(0) for o in physicalSendReceiveNodes])
dataScat = ax.scatter([], [], zorder=40, marker='$\u2709$', s=150, c='r')
dummyDataScat = ax.scatter([], [], zorder=30, marker='.', s=100, c=[(.6, .6, .6, .7)])

#Write down info texts
roundText = ax.text(13.5,.25, "", fontsize=12, bbox=dict(color='white', alpha=1))
phaseText = ax.text(.5,15, "", fontsize=20, bbox=dict(color='white', alpha=1))

#TODO https://stackoverflow.com/questions/7908636/possible-to-make-labels-appear-when-hovering-over-a-point-in-matplotlib

class Phase: #group of objects and an interval s.t. all objects move exactly during that period and vanish before and after
    def __init__(self, first_time, last_time, dummyColor=(.6,.6,.6,.7), name="Anonymous Phase", roundnum=-1):
        self._first_time = first_time #at first timestamp, elements start at their x,y
        self._last_time = last_time #at last timestamp, elements are at targetx, targety
        self._msgs = list() #list containing tuples (x,y,targetx,targety)
        self._dummys = list() #same
        self._dummyColor = dummyColor
        self._name = name
        self._roundnum = roundnum
        
    def addMsg(self, xy, targetxy): #sends message from (x,y) to (targetx, targety)
        self._msgs.append((xy[0],xy[1],targetxy[0],targetxy[1]))
    
    def addDummy(self, xy, targetxy): #sends dummy from (x,y) to (targetx, targety)
        self._dummys.append((xy[0],xy[1],targetxy[0],targetxy[1]))
        
    def finalize(self): #call after all msgs and dummys have been added and before you add it to the phase list
        self._msgdata = dict()
        self._dummydata = dict()
        for (s, t) in [(self._msgs, self._msgdata), (self._dummys, self._dummydata)]:
            t['xy'] = np.array([[x,y] for (x,y,targetx,targety) in s], dtype=float)
            t['delta'] = np.array([[targetx-x, targety-y] for (x,y,targetx,targety) in s], dtype=float)#/(self._last_time-self._first_time)
            t['noise'] = (np.random.rand(len(s), 2)-.5)*.8
            
    def isActive(self, time):
        return time >= self._first_time and time <= self._last_time
    
    def getDummyColor(self):
        return self._dummyColor

currentPhases = list()
previousPhase = None

#Variables for the movement of nodes
reconfFirsttime = -1
reconfLasttime = 0
oldRoutingNodeLocations = np.array([node.getCurrentXY(0) for node in physicalRoutingNodes])
newRoutingNodeLocations = oldRoutingNodeLocations
nextRoutingNodeLocations = np.copy(oldRoutingNodeLocations)
routingNodeLocationsDelta = newRoutingNodeLocations-oldRoutingNodeLocations
routingNodeLocationResetForce = True
routingNodeLocationResetAfterArrivalMustHappen = True

def moveRoutingNodes(first_time, last_time):
    global reconfFirsttime, reconfLasttime, oldRoutingNodeLocations, newRoutingNodeLocations, nextRoutingNodeLocations, routingNodeLocationsDelta, routingNodeLocationResetForce, routingNodeLocationResetAfterArrivalMustHappen
    reconfLasttime = 0
    oldRoutingNodeLocations = newRoutingNodeLocations
    newRoutingNodeLocations = nextRoutingNodeLocations
    nextRoutingNodeLocations = np.copy(newRoutingNodeLocations)
    routingNodeLocationsDelta = newRoutingNodeLocations-oldRoutingNodeLocations
    reconfFirsttime = first_time
    reconfLasttime = last_time
    routingNodeLocationResetForce = True
    routingNodeLocationResetAfterArrivalMustHappen = True
    
def getLocationOfRoutingNode(node, time):
    node = int(node)
    if time >= reconfFirsttime and time <= reconfLasttime:
        phasetime = time - reconfFirsttime
        phaseprogressLinear = phasetime/(reconfLasttime - reconfFirsttime)
        return oldRoutingNodeLocations[node] + phaseprogressLinear * routingNodeLocationsDelta[node]
    elif time < reconfFirsttime:
        return oldRoutingNodeLocations[node]
    elif time > reconfLasttime:
        return nextRoutingNodeLocations[node]


def sigmoid(x): #function for x between -1 and 1, with sigmoid(-1) = 0 and sigmoid(1) = 1. Used to smooth movement
    return x/(1+x**2) + .5

currentFrame = 0
def update(frame):
    global routingNodeLocationResetForce, routingNodeLocationResetAfterArrivalMustHappen, previousPhase, currentFrame
    currentFrame = frame
    t = getCurrentTime()
    
    if saveAnimation:
        if t > reconfLasttime + 1000 and t > max([p._last_time for p in currentPhases], default=0) + 1000:
            injectNextRoundDataFromDump()
    
    #stop_profile()
    #start_profile()
    
    #Physical nodes' movement
    if t >= reconfFirsttime and t <= reconfLasttime:
        phasetime = t - reconfFirsttime
        phaseprogressLinear = phasetime/(reconfLasttime - reconfFirsttime)
        physNodeScat.set_offsets(oldRoutingNodeLocations + phaseprogressLinear * routingNodeLocationsDelta)
    elif routingNodeLocationResetForce and t < reconfLasttime:
        physNodeScat.set_offsets(oldRoutingNodeLocations)
        routingNodeLocationResetForce = False
    elif t > reconfLasttime and routingNodeLocationResetAfterArrivalMustHappen:
        physNodeScat.set_offsets(newRoutingNodeLocations)
        routingNodeLocationResetAfterArrivalMustHappen = False

    #Data objects
    phase = next( (p for p in currentPhases if p.isActive(t)), None)
    if phase is not None:
        phasetime = t - phase._first_time
        phaseprogressLinear = phasetime/(phase._last_time - phase._first_time)
        phaseprogress = sigmoid(2*phaseprogressLinear-1) #how far along on their path are the nodes?. Sigmoid curve to make start and landing a bit slower than the travel
        noiselevel = phaseprogress if phaseprogress < .5 else 1-phaseprogress
        
        for (scat, phasedata) in [(dataScat, phase._msgdata), (dummyDataScat, phase._dummydata)]:
            if len(phasedata['xy']) > 0:
                scat.set_offsets(phasedata['xy'] + phaseprogress * phasedata['delta'] + noiselevel * phasedata['noise'])
            else:
                scat.set_offsets([(-1,-1)])
    else:
        dataScat.set_offsets([(-1, -1)])
        dummyDataScat.set_offsets([(-1, -1)])
        
    #Colors only change between phases, so let's do this whenever the phase changes
    if previousPhase is not phase:
        physSendReceiveNodeScat.set_facecolors([o.getColor(t) for o in physicalSendReceiveNodes]) #colors of send/receive nodes
        physNodeScat.set_facecolors([o.getColor(t) for o in physicalRoutingNodes]) #colors of routing nodes
        if phase is not None:
            dummyDataScat.set_color([phase.getDummyColor()])
            #print(phase._name)
        if phase is not None:
            txt = "Reconfiguration "
            timeLeft = 7-phase._roundnum%7
            if timeLeft == 7:
                txt += "this round"
            if timeLeft == 1:
                txt += "next round"
            if timeLeft < 7 and timeLeft > 1:
                txt += "in "+str(timeLeft)
            
            roundText.set_text(txt.ljust(30, ' '))
        phaseText.set_text((phase._name if phase is not None else "").ljust(30, ' '))
        phaseText.set_color(phase.getDummyColor() if phase is not None else (0,0,0,1))
        previousPhase = phase

    return [ax]

currentRound = args.skip
def injectNextRoundDataFromDump(): #used if saveAnimation is True to replay the dump data
    global currentRound
    currentRound += 1
    filename = savedir+"/round"+str(currentRound)+".txt"
    if os.path.isfile(filename):
        print("Animating "+filename)
        try:
            with open(filename, "r") as text_file:
                r = requests.post("http://localhost:8081", data=text_file)
        except Exception as e:
            print("Error on sending data to visualize.py: ", e)

    

if saveAnimation:
    fps = 60
    fpms = fps/1000
    animation = FuncAnimation(fig, update, frames=fps*saveduration, interval=1000/fps)
    Writer = matplotlib.animation.writers['ffmpeg']
    writer = Writer(fps=fps, metadata=dict(artist='SFB'))
    animation.save('out.mp4', writer=writer)
else:
    animation = FuncAnimation(fig, update, interval=1, blit=True if useBlitting else False) #, init_func=lambda: update(0)
    plt.show()
