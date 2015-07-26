#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <functional>
#include <cstring>
#include <memory>

class TDMS_segment;
class TDMS_segment_object;

class TDMS_data_type
{
public:
    typedef std::function<void* ()> parse_t;
    TDMS_data_type(const TDMS_data_type& dt)
        : name(dt.name),
          read_to(dt.read_to),
          length(dt.length),
          ctype_length(dt.ctype_length)
    {
    }
    TDMS_data_type()
        : name("INVALID TYPE"),
          length(0),
          ctype_length(0)
    {
    }
    TDMS_data_type(const std::string& _name, 
            const size_t _len,
            std::function<void* (const unsigned char*, void*)> reader)
        : name(_name),
          read_to(reader),
          length(_len),
          ctype_length(_len)
    {
    }

    TDMS_data_type(const std::string& _name, 
            const size_t _len,
            const size_t _ctype_len,
            std::function<void* (const unsigned char*, void*)> reader)
        : name(_name),
          read_to(reader),
          length(_len),
          ctype_length(_ctype_len)
    {
    }

    bool is_valid() const
    {
        return (name != "INVALID TYPE");
    }

    bool operator== (const TDMS_data_type& dt) const
    {
        return (name == dt.name);
    }
    bool operator!= (const TDMS_data_type& dt) const
    {
        return !(*this == dt);
    }

    std::string name;
    void* read(const unsigned char* data)
    {
        void* d = malloc(ctype_length);
        read_to(data, d);
        return d;
    }
    std::function<void* (const unsigned char*, void*)> read_to;
    size_t length;
    size_t ctype_length;

    static const std::map<uint32_t, const TDMS_data_type> _tds_datatypes;
};

class TDMS_object
{
    friend class TDMS_file;
    friend class TDMS_segment;
    friend class TDMS_segment_object;
private:
    TDMS_object(const std::string& path)
        : _path(path)
    {
        _data = nullptr;
        _number_values = 0;
        _data_insert_position = 0;
        _previous_segment_object = nullptr;
    }
    void _initialise_data();
    TDMS_segment_object* _previous_segment_object;

    const std::string _path;
    bool _has_data;

    TDMS_data_type _data_type;

    void* _data;
    size_t _data_insert_position;

    struct property{
        property(const TDMS_data_type& dt, void* val)
            : data_type(dt),
              value(val)
        {
        }
        property(const property& p) = delete;
        const TDMS_data_type data_type;
        void* value;
        virtual ~property();
    };

    std::map<std::string, std::unique_ptr<property>> _properties;

    size_t _number_values;

    ~TDMS_object()
    {
        if(_data != nullptr)
            free(_data);
    }
};

class TDMS_file
{
    friend class TDMS_segment;
public:
    TDMS_file(const std::string& filename);
    virtual ~TDMS_file();
private:

    void _parse_segments();

    unsigned char* file_contents;
    size_t file_contents_size;
    std::vector<TDMS_segment*> _segments;

    std::map<std::string, TDMS_object*> _objects;
};
