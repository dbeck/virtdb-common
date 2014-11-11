#include "column_chunk.hh"

#include <string>
#include <logger.hh>
#include <lz4/lz4.h>
#include <util/exception.hh>

using namespace virtdb::engine;

column_chunk::~column_chunk()
{
    delete column_data;
}

int column_chunk::size()
{
    switch (column_data->data().type())
    {
        case virtdb::interface::pb::Kind::STRING:
        case virtdb::interface::pb::Kind::DATE:
        case virtdb::interface::pb::Kind::TIME:
        case virtdb::interface::pb::Kind::DATETIME:
        case virtdb::interface::pb::Kind::NUMERIC:
        case virtdb::interface::pb::Kind::INET4:
        case virtdb::interface::pb::Kind::INET6:
        case virtdb::interface::pb::Kind::MAC:
        case virtdb::interface::pb::Kind::GEODATA:
            return virtdb::util::value_type<std::string>::size(*column_data->mutable_data());
        case virtdb::interface::pb::Kind::INT32:
            return virtdb::util::value_type<int32_t>::size(*column_data->mutable_data());
        case virtdb::interface::pb::Kind::INT64:
            return virtdb::util::value_type<int64_t>::size(*column_data->mutable_data());
        case virtdb::interface::pb::Kind::UINT32:
            return virtdb::util::value_type<uint32_t>::size(*column_data->mutable_data());
        case virtdb::interface::pb::Kind::UINT64:
            return virtdb::util::value_type<uint64_t>::size(*column_data->mutable_data());
        case virtdb::interface::pb::Kind::DOUBLE:
            return virtdb::util::value_type<double>::size(*column_data->mutable_data());
        case virtdb::interface::pb::Kind::FLOAT:
            return virtdb::util::value_type<float>::size(*column_data->mutable_data());
        case virtdb::interface::pb::Kind::BOOL:
            return virtdb::util::value_type<bool>::size(*column_data->mutable_data());
        case virtdb::interface::pb::Kind::BYTES:
            return virtdb::util::value_type<std::string, virtdb::interface::pb::Kind::BYTES>::size(*column_data->mutable_data());
        default:
            LOG_TRACE("We should not be here now.");
            return 0; // TODO why isn't there a template for this?
    }
}

void column_chunk::uncompress()
{
    if (column_data->comptype() == virtdb::interface::pb::CompressionType::LZ4_COMPRESSION)
    {
        int maxDecompressedSize = column_data->uncompressedsize();
        char* destinationBuffer = new char[maxDecompressedSize];
        LZ4_decompress_safe(column_data->compresseddata().c_str(), destinationBuffer, column_data->compresseddata().size(), maxDecompressedSize);
        virtdb::interface::pb::ValueType uncompressed_data;
        uncompressed_data.ParseFromArray(destinationBuffer, maxDecompressedSize);
        column_data->mutable_data()->MergeFrom(uncompressed_data);
        delete [] destinationBuffer;
    }

}

void column_chunk::operator=(virtdb::interface::pb::Column* right_operand)
{
    if (column_data != nullptr)
    {
        std::string err = std::string("Overwriting column_data") + column_data->name() + ", " + std::to_string(column_data->seqno());
        THROW_(err.c_str());
    }
    column_data = right_operand;
}

void column_chunk::operator=(const column_chunk& source)
{
    LOG_TRACE("In operator=, i knew it");
    if (source.column_data != nullptr)
    {
        delete column_data;
        column_data = new virtdb::interface::pb::Column();
        column_data->MergeFrom(*source.column_data);
    }
}

virtdb::interface::pb::Kind column_chunk::get_type()
{
    return column_data->data().type();
}
