#include <fstream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <thread>
#include <vector>
#include <cmath>
#include <algorithm>

namespace py = pybind11;

typedef Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMatrixXi;

typedef Eigen::Matrix<short, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMatrixXs;

void ThreadedRead(Eigen::Ref<RowMatrixXi> data, Eigen::Ref<RowMatrixXs> statChan, uint8_t* buffer, uint32_t start, uint32_t end, uint16_t nChannels, uint16_t nSampRec, uint32_t statusChanIdx){
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

void ReadChannels( Eigen::Ref<RowMatrixXi> data, Eigen::Ref<RowMatrixXs> statChan, char* filename, 
                   int starttime, int endtime, int nChannels, int nSampRec, int statusChanIdx)
{
    std::ifstream file = std::ifstream(filename, std::ifstream::in | std::ifstream::binary);

    assert(file.is_open());

    uint32_t nRec = endtime - starttime;
    uint32_t startpos = (nChannels + 1) * 256 + starttime * nSampRec * 3 * nChannels;

    file.seekg(0, file.end);
    
    size_t length = file.tellg() - startpos;
    
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

    // size_t pos = 0;
    // for(long k = 0; k < nRec; k++)
    // {
    //     for(int ch = 0; ch < nChannels; ch++)
    //     {
    //         if(ch != statusChanIdx){
    //             for(int m = 0; m < nSampRec; m++){
    //                 data(ch, (k * nSampRec) + m) = ((buffer[pos] << 8) |
    //                                                (buffer[pos + 1] << 16) |
    //                                                (buffer[pos + 2] << 24)) >> 8;
    //                 pos += 3;
    //             }
    //         } else {
    //             for(int m = 0; m < nSampRec; m++){
    //                 statChan(0, (k * nSampRec) + m) = buffer[pos];
    //                 statChan(1, (k * nSampRec) + m) = buffer[pos + 1];
    //                 statChan(2, (k * nSampRec) + m) = buffer[pos + 2];
    //                 pos += 3;
    //             }
    //         }
    //     }
    // }

    free(buffer);
}

//constexpr bool rowMajor = Matrix::Flags & Eigen::RowMajorBit;

PYBIND11_MODULE(libcppbdf,m)
{
  m.doc() = "pybdf c++ version dll";

//   py::class_<Matrix>(m, "Matrix", py::buffer_protocol())
//     .def("__init__", [](Matrix &m, py::buffer b) {
//         typedef Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic> Strides;

//         /* Request a buffer descriptor from Python */
//         py::buffer_info info = b.request();

//         /* Some sanity checks ... */
//         if (info.ndim != 2)
//             throw std::runtime_error("Incompatible buffer dimension!");

//         auto strides = Strides(
//             info.strides[rowMajor ? 0 : 1] / (py::ssize_t)sizeof(int),
//             info.strides[rowMajor ? 1 : 0] / (py::ssize_t)sizeof(int));

//         auto map = Eigen::Map<Matrix, 0, Strides>(
//             static_cast<int *>(info.ptr), info.shape[0], info.shape[1], strides);

//         new (&m) Matrix(map);
//     })
//     .def_buffer([](Matrix &m) -> py::buffer_info {
//         return py::buffer_info(
//             m.data(),                                   /* Pointer to buffer */
//             sizeof(int),                                /* Size of one scalar */
//             py::format_descriptor<int>::format(),       /* Python struct-style format descriptor */
//             2,                                          /* Number of dimensions */
//             { m.rows(), m.cols() },                     /* Buffer dimensions */
//             { sizeof(int) * (rowMajor ? m.cols() : 1),
//             sizeof(int) * (rowMajor ? 1 : m.rows()) }
//                                                         /* Strides (in bytes) for each index */
//         );
//     }); // End class matrix

  m.def("read_channels", &ReadChannels);
}