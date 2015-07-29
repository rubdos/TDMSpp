#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <functional>
#include <memory>

namespace TDMS
{

class file;
class object;
class segment_object;

enum endianness
{
    BIG,
    LITTLE
};

class segment
{
    friend class file;
    friend class object;
    friend class segment_object;
private:
    class no_segment_error : public std::runtime_error
    {
    public:
        no_segment_error() 
            : std::runtime_error("Not a segment")
        {
        }
    };
    typedef segment_object object;

    segment(const unsigned char* file_contents, 
            segment* previous_segment,
            file* file);
    virtual ~segment();

    void _parse_metadata(const unsigned char* data, 
            segment* previous_segment);
    void _parse_raw_data();
    void _calculate_chunks();

    size_t _offset;
    size_t _chunk_count;

    // Probably a map using enums performs faster.
    // Will only give a little performance though.
    // Perhaps use a struct for the _toc, so we don't need
    // the std::map and don't have to do lookups.
    std::map<std::string, bool> _toc;
    const unsigned char* _data;
    size_t _next_segment_offset;
    size_t _raw_data_offset;
    size_t _num_chunks;
    std::vector<std::shared_ptr<segment::object>> _ordered_objects;

    file* _parent_file;

    static const std::map<const std::string, int32_t> _toc_properties;
};

class segment_object
{
    friend class segment;
    friend class object;
private:
    segment_object(object* o);
    const unsigned char* _parse_metadata(const unsigned char* data);
    void _read_values(const unsigned char*& data, endianness e);
    object* _tdms_object;

    uint64_t _number_values;
    uint64_t _data_size;
    bool _has_data;
    uint32_t _dimension;
    data_type_t _data_type;
    //_dimension;
};
}
