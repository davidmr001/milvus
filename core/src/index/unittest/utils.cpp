// Copyright (C) 2019-2020 Zilliz. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied. See the License for the specific language governing permissions and limitations under the License.

#include "unittest/utils.h"
#include "knowhere/index/vector_index/adapter/VectorAdapter.h"

#include <gtest/gtest.h>
#include <math.h>
#include <memory>
#include <string>
#include <utility>

INITIALIZE_EASYLOGGINGPP

void
InitLog() {
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.set(el::Level::Debug, el::ConfigurationType::Format, "[%thread-%datetime-%level]: %msg (%fbase:%line)");
    el::Loggers::reconfigureLogger("default", defaultConf);
}

void
DataGen::Init_with_default() {
    Generate(dim, nb, nq);
}

void
BinaryDataGen::Init_with_binary_default() {
    Generate(dim, nb, nq);
}

void
DataGen::Generate(const int& dim, const int& nb, const int& nq) {
    this->nb = nb;
    this->nq = nq;
    this->dim = dim;

    GenAll(dim, nb, xb, ids, xids, nq, xq);
    assert(xb.size() == (size_t)dim * nb);
    assert(xq.size() == (size_t)dim * nq);

    base_dataset = generate_dataset(nb, dim, xb.data(), ids.data());
    query_dataset = generate_query_dataset(nq, dim, xq.data());
    id_dataset = generate_id_dataset(nq, ids.data());
    xid_dataset = generate_id_dataset(nq, xids.data());
    xid_dataset->Set(milvus::knowhere::meta::DIM, (int64_t)dim);
}

void
BinaryDataGen::Generate(const int& dim, const int& nb, const int& nq) {
    this->nb = nb;
    this->nq = nq;
    this->dim = dim;

    int64_t dim_x = dim / 8;
    GenBinaryAll(dim_x, nb, xb, ids, xids, nq, xq);
    assert(xb.size() == (size_t)dim_x * nb);
    assert(xq.size() == (size_t)dim_x * nq);

    base_dataset = generate_dataset(nb, dim, xb.data(), ids.data());
    query_dataset = generate_query_dataset(nq, dim, xq.data());
    id_dataset = generate_id_dataset(nq, ids.data());
    xid_dataset = generate_id_dataset(nq, xids.data());
}

// not used
#if 0
knowhere::DatasetPtr
DataGen::GenQuery(const int& nq) {
    xq.resize(nq * dim);
    for (int i = 0; i < nq * dim; ++i) {
        xq[i] = xb[i];
    }
    return generate_query_dataset(nq, dim, xq.data());
}
#endif

void
GenAll(const int64_t dim, const int64_t& nb, std::vector<float>& xb, std::vector<int64_t>& ids,
       std::vector<int64_t>& xids, const int64_t& nq, std::vector<float>& xq) {
    xb.resize(nb * dim);
    xq.resize(nq * dim);
    ids.resize(nb);
    xids.resize(1);
    GenAll(dim, nb, xb.data(), ids.data(), xids.data(), nq, xq.data());
}

void
GenAll(const int64_t& dim, const int64_t& nb, float* xb, int64_t* ids, int64_t* xids, const int64_t& nq, float* xq) {
    GenBase(dim, nb, xb, ids);
    for (int64_t i = 0; i < nq * dim; ++i) {
        xq[i] = xb[i];
    }
    xids[0] = 3;  // pseudo random
}

void
GenBinaryAll(const int64_t dim, const int64_t& nb, std::vector<uint8_t>& xb, std::vector<int64_t>& ids,
             std::vector<int64_t>& xids, const int64_t& nq, std::vector<uint8_t>& xq) {
    xb.resize(nb * dim);
    xq.resize(nq * dim);
    ids.resize(nb);
    xids.resize(1);
    GenBinaryAll(dim, nb, xb.data(), ids.data(), xids.data(), nq, xq.data());
}

void
GenBinaryAll(const int64_t& dim, const int64_t& nb, uint8_t* xb, int64_t* ids, int64_t* xids, const int64_t& nq,
             uint8_t* xq) {
    GenBinaryBase(dim, nb, xb, ids);
    for (int64_t i = 0; i < nq * dim; ++i) {
        xq[i] = xb[i];
    }
    xids[0] = 3;  // pseudo random
}

void
GenBase(const int64_t& dim, const int64_t& nb, float* xb, int64_t* ids) {
    for (auto i = 0; i < nb; ++i) {
        for (auto j = 0; j < dim; ++j) {
            // p_data[i * d + j] = float(base + i);
            xb[i * dim + j] = drand48();
        }
        xb[dim * i] += i / 1000.;
        ids[i] = i;
    }
}

void
GenBinaryBase(const int64_t& dim, const int64_t& nb, uint8_t* xb, int64_t* ids) {
    for (auto i = 0; i < nb; ++i) {
        for (auto j = 0; j < dim; ++j) {
            // p_data[i * d + j] = float(base + i);
            xb[i * dim + j] = (uint8_t)lrand48();
        }
        ids[i] = i;
    }
}

FileIOReader::FileIOReader(const std::string& fname) {
    name = fname;
    fs = std::fstream(name, std::ios::in | std::ios::binary);
}

FileIOReader::~FileIOReader() {
    fs.close();
}

size_t
FileIOReader::operator()(void* ptr, size_t size) {
    fs.read(reinterpret_cast<char*>(ptr), size);
    return size;
}

FileIOWriter::FileIOWriter(const std::string& fname) {
    name = fname;
    fs = std::fstream(name, std::ios::out | std::ios::binary);
}

FileIOWriter::~FileIOWriter() {
    fs.close();
}

size_t
FileIOWriter::operator()(void* ptr, size_t size) {
    fs.write(reinterpret_cast<char*>(ptr), size);
    return size;
}

milvus::knowhere::DatasetPtr
generate_dataset(int64_t nb, int64_t dim, const void* xb, const int64_t* ids) {
    auto ret_ds = std::make_shared<milvus::knowhere::Dataset>();
    ret_ds->Set(milvus::knowhere::meta::ROWS, nb);
    ret_ds->Set(milvus::knowhere::meta::DIM, dim);
    ret_ds->Set(milvus::knowhere::meta::TENSOR, xb);
    ret_ds->Set(milvus::knowhere::meta::IDS, ids);
    return ret_ds;
}

milvus::knowhere::DatasetPtr
generate_query_dataset(int64_t nb, int64_t dim, const void* xb) {
    auto ret_ds = std::make_shared<milvus::knowhere::Dataset>();
    ret_ds->Set(milvus::knowhere::meta::ROWS, nb);
    ret_ds->Set(milvus::knowhere::meta::DIM, dim);
    ret_ds->Set(milvus::knowhere::meta::TENSOR, xb);
    return ret_ds;
}

milvus::knowhere::DatasetPtr
generate_id_dataset(int64_t nb, const int64_t* ids) {
    auto ret_ds = std::make_shared<milvus::knowhere::Dataset>();
    ret_ds->Set(milvus::knowhere::meta::ROWS, nb);
    ret_ds->Set(milvus::knowhere::meta::IDS, ids);
    return ret_ds;
}

void
AssertAnns(const milvus::knowhere::DatasetPtr& result, const int nq, const int k, const CheckMode check_mode) {
    auto ids = result->Get<int64_t*>(milvus::knowhere::meta::IDS);
    for (auto i = 0; i < nq; i++) {
        switch (check_mode) {
            case CheckMode::CHECK_EQUAL:
                ASSERT_EQ(i, *((int64_t*)(ids) + i * k));
                break;
            case CheckMode::CHECK_NOT_EQUAL:
                ASSERT_NE(i, *((int64_t*)(ids) + i * k));
                break;
            default:
                ASSERT_TRUE(false);
                break;
        }
    }
}

void
AssertVec(const milvus::knowhere::DatasetPtr& result, const milvus::knowhere::DatasetPtr& base_dataset,
          const milvus::knowhere::DatasetPtr& id_dataset, const int n, const int dim, const CheckMode check_mode) {
    float* base = (float*)base_dataset->Get<const void*>(milvus::knowhere::meta::TENSOR);
    auto ids = id_dataset->Get<const int64_t*>(milvus::knowhere::meta::IDS);
    auto x = result->Get<float*>(milvus::knowhere::meta::TENSOR);
    for (auto i = 0; i < n; i++) {
        auto id = ids[i];
        for (auto j = 0; j < dim; j++) {
            switch (check_mode) {
                case CheckMode::CHECK_EQUAL: {
                    ASSERT_EQ(*(base + id * dim + j), *(x + i * dim + j));
                    break;
                }
                case CheckMode::CHECK_NOT_EQUAL: {
                    ASSERT_NE(*(base + id * dim + j), *(x + i * dim + j));
                    break;
                }
                case CheckMode::CHECK_APPROXIMATE_EQUAL: {
                    float a = *(base + id * dim + j);
                    float b = *(x + i * dim + j);
                    ASSERT_TRUE((std::fabs(a - b) / std::fabs(a)) < 0.1);
                    break;
                }
                default:
                    ASSERT_TRUE(false);
                    break;
            }
        }
    }
}

void
AssertBinVeceq(const milvus::knowhere::DatasetPtr& result, const milvus::knowhere::DatasetPtr& base_dataset,
               const milvus::knowhere::DatasetPtr& id_dataset, const int n, const int dim) {
    auto base = base_dataset->Get<const uint8_t*>(milvus::knowhere::meta::TENSOR);
    auto ids = id_dataset->Get<const int64_t*>(milvus::knowhere::meta::IDS);
    auto x = result->Get<uint8_t*>(milvus::knowhere::meta::TENSOR);
    for (auto i = 0; i < 1; i++) {
        auto id = ids[i];
        for (auto j = 0; j < dim; j++) {
            EXPECT_EQ(*(base + id * dim + j), *(x + i * dim + j));
        }
    }
}

void
PrintResult(const milvus::knowhere::DatasetPtr& result, const int& nq, const int& k) {
    auto ids = result->Get<int64_t*>(milvus::knowhere::meta::IDS);
    auto dist = result->Get<float*>(milvus::knowhere::meta::DISTANCE);

    std::stringstream ss_id;
    std::stringstream ss_dist;
    for (auto i = 0; i < nq; i++) {
        for (auto j = 0; j < k; ++j) {
            // ss_id << *(ids->data()->GetValues<int64_t>(1, i * k + j)) << " ";
            // ss_dist << *(dists->data()->GetValues<float>(1, i * k + j)) << " ";
            ss_id << *((int64_t*)(ids) + i * k + j) << " ";
            ss_dist << *((float*)(dist) + i * k + j) << " ";
        }
        ss_id << std::endl;
        ss_dist << std::endl;
    }
    std::cout << "id\n" << ss_id.str() << std::endl;
    std::cout << "dist\n" << ss_dist.str() << std::endl;
}

// not used
#if 0
void
Load_nns_graph(std::vector<std::vector<int64_t>>& final_graph, const char* filename) {
    std::vector<std::vector<unsigned>> knng;

    std::ifstream in(filename, std::ios::binary);
    unsigned k;
    in.read((char*)&k, sizeof(unsigned));
    in.seekg(0, std::ios::end);
    std::ios::pos_type ss = in.tellg();
    size_t fsize = (size_t)ss;
    size_t num = (size_t)(fsize / (k + 1) / 4);
    in.seekg(0, std::ios::beg);

    knng.resize(num);
    knng.reserve(num);
    int64_t kk = (k + 3) / 4 * 4;
    for (size_t i = 0; i < num; i++) {
        in.seekg(4, std::ios::cur);
        knng[i].resize(k);
        knng[i].reserve(kk);
        in.read((char*)knng[i].data(), k * sizeof(unsigned));
    }
    in.close();

    final_graph.resize(knng.size());
    for (int i = 0; i < knng.size(); ++i) {
        final_graph[i].resize(knng[i].size());
        for (int j = 0; j < knng[i].size(); ++j) {
            final_graph[i][j] = knng[i][j];
        }
    }
}

float*
fvecs_read(const char* fname, size_t* d_out, size_t* n_out) {
    FILE* f = fopen(fname, "r");
    if (!f) {
        fprintf(stderr, "could not open %s\n", fname);
        perror("");
        abort();
    }
    int d;
    fread(&d, 1, sizeof(int), f);
    assert((d > 0 && d < 1000000) || !"unreasonable dimension");
    fseek(f, 0, SEEK_SET);
    struct stat st;
    fstat(fileno(f), &st);
    size_t sz = st.st_size;
    assert(sz % ((d + 1) * 4) == 0 || !"weird file size");
    size_t n = sz / ((d + 1) * 4);

    *d_out = d;
    *n_out = n;
    float* x = new float[n * (d + 1)];
    size_t nr = fread(x, sizeof(float), n * (d + 1), f);
    assert(nr == n * (d + 1) || !"could not read whole file");

    // shift array to remove row headers
    for (size_t i = 0; i < n; i++) memmove(x + i * d, x + 1 + i * (d + 1), d * sizeof(*x));

    fclose(f);
    return x;
}

int*  // not very clean, but works as long as sizeof(int) == sizeof(float)
ivecs_read(const char* fname, size_t* d_out, size_t* n_out) {
    return (int*)fvecs_read(fname, d_out, n_out);
}
#endif
