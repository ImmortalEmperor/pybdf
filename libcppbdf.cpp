#include <fstream>
#include <thread>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "pybind11/numpy.h"
#include "pybind11/eigen.h"

typedef unsigned short ushort;

namespace py = pybind11;

typedef Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMatrixXi;

typedef Eigen::Matrix<uint8_t, 3, Eigen::Dynamic, Eigen::RowMajor> RowMatrix3ui;

void ThreadedRead(Eigen::Ref<RowMatrixXi> data, Eigen::Ref<RowMatrix3ui> statChan, uint8_t* buffer, uint32_t start, uint32_t end, uint16_t nChannels, uint16_t nSampRec, uint32_t statusChanIdx){
    uint32_t pos = start * 3 * nChannels * nSampRec;
    for(int k = start; k < end; k++){
        for(int ch = 0; ch < nChannels; ch++)
        {
            if(ch != statusChanIdx){
                for(int m = 0; m < nSampRec; m++){
                    data(ch, (k * nSampRec) + m) = ((buffer[pos] << 8) |
                                                   (buffer[pos + 1] << 16) |
                                                   (buffer[pos + 2] << 24)) >> 8;
                    pos += 3;
                }
            } else {
                for(int m = 0; m < nSampRec; m++){
                    statChan(0, (k * nSampRec) + m) = buffer[pos];
                    statChan(1, (k * nSampRec) + m) = buffer[pos + 1];
                    statChan(2, (k * nSampRec) + m) = buffer[pos + 2];
                    pos += 3;
                }
            }
        }
    }
}

void ReadChannels( Eigen::Ref<RowMatrixXi> data, Eigen::Ref<RowMatrix3ui> statChan, char* filename, 
                   int starttime, int endtime, int nChannels, int nSampRec, int statusChanIdx)
{
    std::ifstream file (filename, std::ifstream::in | std::ifstream::binary);

    assert(file.is_open());

    uint32_t nRec = endtime - starttime;
    uint32_t startpos = (nChannels + 1) * 256 + starttime * nSampRec * 3 * nChannels;

    file.seekg(0, file.end);
    
    size_t length = uint32_t(file.tellg()) - startpos;
    
    file.seekg(0, file.beg); 
    file.clear();

    file.seekg(startpos);

    uint8_t* buffer = (uint8_t*)malloc(length);

    file.read((char*)buffer, length);

    file.close();

    uint32_t concurentThreadsSupported = std::max((uint32_t)4, std::thread::hardware_concurrency());

    uint32_t numPerThread = std::ceil(float(nRec) / float(concurentThreadsSupported));

    std::vector<std::thread> threadpool;
    for(uint32_t i = 0; i < concurentThreadsSupported; i++)
    {
        threadpool.push_back(std::thread(ThreadedRead, data, statChan, buffer, i * numPerThread, std::min(nRec, (i + 1) * numPerThread), nChannels, nSampRec , statusChanIdx));
    }

    for(auto& t : threadpool){
        t.join();
    }

    free(buffer);
}


//TODO switch writing to do it in memory AKA read whole file to buffer, edit with threads, then output buffer back to file
void WriteTriggers(py::array_t<uint8_t> code, py::array_t<int64_t> idx, char* filename, 
                   const int duration, const int nChannels, const int nSampRec,  const int statusChanIdx)
{
    auto buf_code = code.request();
    auto buf_idx = idx.request();
    
    if(buf_code.ndim != 1 || buf_idx.ndim != 1){
        std::cout << "Too many dim got: " << buf_code.ndim << " and: " << buf_idx.ndim << std::endl;
        exit(-1);
    }

    if(buf_code.shape[0] != buf_idx.shape[0]){
        std::cout << "Code shape[0]: " << buf_code.shape[0] << "does not match idx shape[0]: " << buf_idx.shape[0] << std::endl;
        exit(-1);
    }

    std::cout << "Updating file of " << duration << " seconds " << nChannels << " channels " << nSampRec << " records " << std::endl;

    std::cout << "Recieved " << buf_code.shape[0] << " codes at " << buf_idx.shape[0] << " indices" << std::endl;

    std::fstream out_file(filename, std::ios_base::binary | std::ios_base::out | std::ios_base::in);

    assert(out_file.is_open());

    uint32_t startpos = (nChannels + 1) * 256;

    out_file.seekp(out_file.beg);

    //seek to first trigger block
    out_file.seekp(startpos + statusChanIdx * 3 * nSampRec);

    uint32_t i = 0; 
    uint32_t current = 0;
    bool bWriteZero = true;
    uint8_t zero = 0;

    uint64_t* idxPtr = (uint64_t*)buf_idx.ptr;
    uint8_t* codePtr = (uint8_t*)buf_code.ptr;

    while(i < duration)
    {
        for(ushort s = 0; s < nSampRec; s++)
        {
            bWriteZero = true;
            
            if(current < buf_idx.shape[0])
            {
                if (idxPtr[current] == (i * nSampRec) + s)
                {
                    //std::cout << "Writing code " << int(codePtr[current]) << " from index " << current << " at time " << i * nSampRec + s << " should be " << idxPtr[current] << std::endl;
                    out_file.write((const char*)&codePtr[current], sizeof(uint8_t));
                    bWriteZero = false;
                    current++;
                }
            }
            
            if (bWriteZero)
            {
                out_file.write((const char*)&zero, sizeof(uint8_t));
            }

            //skip next 2 characters
            out_file.seekp(2, std::ios_base::cur);
        }
        
        i++;

        //seek to next trigger block
        out_file.seekp(statusChanIdx * 3 * nSampRec, std::ios_base::cur);
    }
    
    out_file.close();

    if(current < buf_idx.shape[0]){
        std::cout << "Triggers out of data bounds" << std::endl;
    }
    
    std::cout << "-- Done Writing --" << std::endl;

}

PYBIND11_MODULE(libcppbdf,m)
{
  m.doc() = "pybdf c++ version dll";
  m.def("read_channels", &ReadChannels);
  m.def("write_triggers", &WriteTriggers);
}