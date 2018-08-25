import pybdf
from glob import glob
import numpy as np
from shutil import copyfile
import os

def importBDF(filename, useCpp=False):
    """returns dataMatrix, eventTable, chanLabels"""
    if len(filename) > 0:
        rec1 = pybdf.bdfRecording(filename)

        data1 = rec1.getData(trigChan=True, useCpp=useCpp)

        #retrieve sampling rates (list of sampling rate of each channel)
        sampRate1 = rec1.sampRate
        print("**********************")
        print("The sampling rate of, ", filename, "is", sampRate1[0], "Hz")
        print()

        dur1 = rec1.duration
        #retrieve recording durations (list of recording durations of each channel)
        print("**********************")
        print("The duration of, ", filename, "is", dur1, "seconds")
        print()

        dataMatrix = data1['data']
        #time_array1 = np.arange(dataMatrix1.shape[1]) / rec1.sampRate[0]

        chanLabels = rec1.chanLabels 

        eventTable = data1['eventTable']
        
        return dataMatrix, eventTable, chanLabels
    else:
        print("--Please enter a file--")

files = sorted(glob('./*2048*'))

if len(files) > 1:
    os.remove(files.pop(1))

files.append(files[0][0:-4]+'_out.bdf')

print(files)

copyfile(files[0], files[1])

source_file = pybdf.bdfRecording(files[0])

data = source_file.getData()

cpp_event= data['eventTable']

print(cpp_event)

rec_out = pybdf.bdfRecording(files[1])

out_cpp_codes = np.arange(1, cpp_event['code'].shape[0] + 1, 1, dtype=np.uint8)
out_cpp_times = (cpp_event['idx'] + 1)

#rec_out.write_triggers(out_cpp_codes, out_cpp_times)

test_cpp_data, test_cpp_event, test_cpp_chan = importBDF(files[1], useCpp=True)

print(test_cpp_event)