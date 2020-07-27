#Start with command: waitress-serve --port=8080 visserver:app

import falcon
import ujson
import json
from queue import Queue
from threading import Thread
import requests
import os

#Set up worker thread
def work():
    while True:
        data = queue.get()
        parseAndForward(data)
        queue.task_done()
        
        
queue = Queue(maxsize=0)
worker = Thread(target=work)
worker.setDaemon(True)
worker.start()


#Set up parseAndForward
dataForRound = dict()
numPhysNodes = 81
lastFullRound = -1
def parseAndForward(data):
    global dataForRound, lastFullRound
    #try:
    #    data = ujson.loads(self.data_string)
    #except:
    #    print("Error decoding")
    #    print(self.data_string)
    #    return
    
    #Store data
    dataround = data['cur_round']
    if dataround not in dataForRound:
        dataForRound[dataround] = dict()
    dataForRound[dataround][data['own_id']] = data
    print("got "+str(data['own_id']) + ", i.e. "+str(len(dataForRound[dataround].values()))+"/"+str(numPhysNodes)+" for round "+str(dataround)+". Missing: "+str(list([x for x in range(numPhysNodes) if x not in dataForRound[dataround]])))

    #Run update as soon as we've got all data
    if len(dataForRound[dataround].values()) >= numPhysNodes and dataround > lastFullRound:
        print("animating")
        forward(dataForRound[dataround], dataround)
        dataForRound[dataround] = dict()
        lastFullRound = dataround

    #Run update if next round already started.
    if dataround > lastFullRound + 1 and dataround-1 in dataForRound and len(dataForRound[dataround-1]) > 0:
        print("animating with incomplete data")
        forward(dataForRound[dataround-1], dataround-1)
        dataForRound[dataround-1] = dict()
        lastFullRound = dataround-1

def forward(data, dataround):
    try:
        r = requests.post("http://localhost:8081", data=ujson.dumps(data))
    except:
        print("Error on sending data to visualize.py")
    
    os.makedirs(os.path.dirname("dumps/dump.txt"), exist_ok=True)
    with open("dumps/dump.txt", "w") as text_file:
        ujson.dump(data, text_file, indent=4, sort_keys=True)
    if dataround < 120:
        filename = "dumps/round"+str(dataround)+".txt"
        os.makedirs(os.path.dirname(filename), exist_ok=True)
        with open(filename, "w") as text_file:
            ujson.dump(data, text_file, indent=4, sort_keys=True)
        for node in data:
            filename = "dumps/node"+str(node)+".txt"
            with open(filename, "w" if dataround == 0 else "a") as text_file:
                ujson.dump(data[node], text_file, indent=4, sort_keys=True)
                text_file.write("---------------------------------------\n")

        
#Set up webserver
class VisDataResource(object): #collects requests and forwards them to visualize.py
    def on_post(self, req, resp):
        data = ujson.load(req.bounded_stream)
        #parseAndForward(data)
        queue.put(data)
        resp.status = falcon.HTTP_200
        resp.body = "Received the data."

# falcon.API instances are callable WSGI apps
app = falcon.API()

# Resources are represented by long-lived class instances
res = VisDataResource()
app.add_route('/', res)
