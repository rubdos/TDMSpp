#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <functional>

class TDMS_file;
class TDMS_object;
class TDMS_segment_object;

enum endianness
{
    BIG,
    LITTLE
};

class TDMS_segment
{
    friend class TDMS_file;
    friend class TDMS_object;
    friend class TDMS_segment_object;
private:
    class no_segment_error : public std::runtime_error
    {
    public:
        no_segment_error() 
            : std::runtime_error("Not a segment")
        {
        }
    };
    typedef TDMS_segment_object object;

    TDMS_segment(const unsigned char* file_contents, 
            TDMS_segment* previous_segment,
            TDMS_file* file);
    virtual ~TDMS_segment();

    void _parse_metadata(const unsigned char* data, 
            TDMS_segment* previous_segment);
    void _parse_raw_data();
    void _calculate_chuncks();

    size_t _offset;
    size_t _chunck_count;

    // Probably a map using enums performs faster.
    // Will only give a little performance though.
    // Perhaps use a struct for the _toc, so we don't need
    // the std::map and don't have to do lookups.
    std::map<std::string, bool> _toc;
    const unsigned char* _data;
    size_t _next_segment_offset;
    size_t _raw_data_offset;
    size_t _num_chunks;
    std::vector<TDMS_segment::object*> _ordered_objects;

    TDMS_file* _parent_file;

    static const std::map<const std::string, int32_t> _toc_properties;
};

class TDMS_segment_object
{
    friend class TDMS_segment;
    friend class TDMS_object;
private:
    TDMS_segment_object(TDMS_object* o);
    const unsigned char* _parse_metadata(const unsigned char* data);
    void _read_values(const unsigned char*& data, endianness e);
    TDMS_object* _tdms_object;

    uint64_t _number_values;
    uint64_t _data_size;
    bool _has_data;
    uint32_t _dimension;
    TDMS_data_type _data_type;
    //_dimension;
};
