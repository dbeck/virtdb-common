#ifdef RELEASE
#undef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED false
#undef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "column_chunk.hh"

#include <string>
#include <logger.hh>
#include <lz4/lib/lz4.h>
#include <util/exception.hh>
#include <util/flex_alloc.hh>

using namespace virtdb::engine;

column_chunk::~column_chunk()
{
}

const std::string &
column_chunk::name() const
{
    static const std::string empty;
    if( column_data )
        return column_data->name();
    else
        return empty;
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
        util::flex_alloc<char, 2048> buffer(maxDecompressedSize);
        char* destinationBuffer = buffer.get();
        int comp_ret = LZ4_decompress_safe(column_data->compresseddata().c_str(),
                                           destinationBuffer,
                                           column_data->compresseddata().size(),
                                           maxDecompressedSize);
        if( comp_ret <= 0 )
        {
            THROW_("failed to decompress data");
        }
        if( !column_data->mutable_data()->ParseFromArray(destinationBuffer, maxDecompressedSize) )
        {
            THROW_("failed to parse decompressed data");
        }
    }
}

column_chunk::column_chunk(std::shared_ptr<virtdb::interface::pb::Column> data)
{
    if (data.get() == nullptr)
    {
        std::string err = std::string("Initializing column_chunk with nullptr as data");
        THROW_(err.c_str());
    }
    column_data = data;
}

void column_chunk::operator=(const column_chunk& source)
{
    if (source.column_data.get() != nullptr)
    {
        column_data = source.column_data;
    }
}

virtdb::interface::pb::Kind column_chunk::get_type()
{
    return column_data->data().type();
}
