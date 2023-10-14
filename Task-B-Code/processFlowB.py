import xml.etree.ElementTree as ET
import os
import pandas as pd

df = pd.DataFrame(columns=['Algo', 'Nodes', 'Error Rate', 'Throughput', 'End to End Delay',
                           'Delivery Ratio', 'Drop Ratio'])

def process(path):
    tree = ET.parse(path)
    root = tree.getroot()

    for child in root:
        totalThroughPut = totalPacketsSent = totalPacketsSent = 0
        totalPacketsReceived = totalDelaySum = 0
        for i, flow in enumerate(child):
            if i>= len(child)/2:
                break
            if int(flow.attrib['rxPackets']) == 0:
                continue
            start = float(flow.attrib['timeFirstRxPacket'][:-2])
            stop = float(flow.attrib['timeLastRxPacket'][:-2])
            duration = (stop-start)*1e-9
            if duration==0:
                continue
            totalThroughPut += int(flow.attrib['rxBytes'])*8 / duration 

            totalPacketsSent += int(flow.attrib['txPackets'])
            totalPacketsReceived += int(flow.attrib['rxPackets'])
            totalDelaySum += float(flow.attrib['delaySum'][:-2])
        break

    totalThroughPut = round(totalThroughPut/1024, 2)
    e2eDelay = round((totalDelaySum/totalPacketsReceived)/1000)
    deliveryRatio = round((totalPacketsReceived / totalPacketsSent)*100, 2)
    dropRatio = round(((totalPacketsSent-totalPacketsReceived) / totalPacketsSent)*100, 2)

    return totalThroughPut, e2eDelay, deliveryRatio, dropRatio


for f in os.listdir():
    if f.endswith(".flowmonitor"):
        l = f.split(".")[0]
        l = l.split("-")
        algo = l[1].strip()
        nodes = int(l[2].strip())
        error_rate = int(l[3].strip())
        path = os.path.join(os.getcwd(), f)
        result = process(path)

        row = {'Algo': algo, 'Nodes': nodes, 'Error Rate': error_rate, 'Throughput': result[0],  
               'End to End Delay': result[1], 'Delivery Ratio': result[2], 
               'Drop Ratio': result[3]}

        df = df.append(pd.Series(row), ignore_index=True)

df.to_csv("results.csv", index=False)

