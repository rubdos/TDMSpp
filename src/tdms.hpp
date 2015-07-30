#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <functional>
#include <cstring>
#include <memory>
#include "log.hpp"

namespace TDMS
{

class segment;
class segment_object;

class data_type_t
{
public:
    typedef std::function<void* ()> parse_t;

    data_type_t(const data_type_t& dt)
        : name(dt.name),
          read_to(dt.read_to),
          read_array_to(dt.read_array_to),
          length(dt.length),
          ctype_length(dt.ctype_length)
    {
    }
    data_type_t()
        : name("INVALID TYPE"),
          length(0),
          ctype_length(0)
    {
        _init_default_array_reader();
    }
    data_type_t(const std::string& _name, 
            const size_t _len,
            std::function<void (const unsigned char*, void*)> reader)
        : name(_name),
          read_to(reader),
          length(_len),
          ctype_length(_len)
    {
        _init_default_array_reader();
    }
    data_type_t(const std::string& _name, 
            const size_t _len,
            std::function<void (const unsigned char*, void*)> reader,
            std::function<void (const unsigned char*, void*, size_t)> array_reader)
        : name(_name),
          read_to(reader),
          read_array_to(array_reader),
          length(_len),
          ctype_length(_len)
    {
    }

    data_type_t(const std::string& _name, 
            const size_t _len,
            const size_t _ctype_len,
            std::function<void (const unsigned char*, void*)> reader)
        : name(_name),
          read_to(reader),
          length(_len),
          ctype_length(_ctype_len)
    {
        _init_default_array_reader();
    }

    bool is_valid() const
    {
        return (name != "INVALID TYPE");
    }

    bool operator== (const data_type_t& dt) const
    {
        return (name == dt.name);
    }
    bool operator!= (const data_type_t& dt) const
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
    std::function<void (const unsigned char*, void*)> read_to;
    std::function<void (const unsigned char*, void*, size_t)> read_array_to;
    size_t length;
    size_t ctype_length;

    static const std::map<uint32_t, const data_type_t> _tds_datatypes;
private:
    void _init_default_array_reader();
};

class object
{
    friend class file;
    friend class segment;
    friend class segment_object;
public:
    struct property{
        property(const data_type_t& dt, void* val)
            : data_type(dt),
              value(val)
        {
        }
        property(const property& p) = delete;
        const data_type_t data_type;
        void* value;
        virtual ~property();
    };

    const std::string data_type()
    {
        return _data_type.name;
    }

    size_t bytes()
    {
        return _data_type.ctype_length * _number_values;
    }

    const void* data()
    {
        return _data;
    }

    size_t number_values()
    {
        return _number_values;
    }

    const std::string get_path()
    {
        return _path;
    }
    const std::map<std::string, std::shared_ptr<property>> get_properties()
    {
        return _properties;
    }
private:
    object(const std::string& path)
        : _path(path)
    {
        _data = nullptr;
        _number_values = 0;
        _data_insert_position = 0;
        _previous_segment_object = nullptr;
    }
    void _initialise_data();
    std::shared_ptr<segment_object> _previous_segment_object;

    const std::string _path;
    bool _has_data;

    data_type_t _data_type;

    void* _data;
    size_t _data_insert_position;

    std::map<std::string, std::shared_ptr<property>> _properties;

    size_t _number_values;

    ~object()
    {
        if(_data != nullptr)
            free(_data);
    }
};

class file
{
    friend class segment;
public:
    file(const std::string& filename);
    virtual ~file();

    const object* operator[](const std::string& key);
    class iterator
    {
        friend class file;
    public:
        object* operator*()
        {
            return _it->second;
        }
        const iterator& operator++()
        {
            ++_it;

            return *this;
        }
        bool operator !=(const iterator& other)
        {
            return other._it != _it;
        }
    private:
        iterator(std::map<std::string, object*>::iterator it)
            : _it(it)
        {}
        std::map<std::string, object*>::iterator _it;
    };
    iterator begin()
    {
        return iterator(_objects.begin());
    }
    iterator end()
    {
        return iterator(_objects.end());
    }
private:

    void _parse_segments();

    unsigned char* file_contents;
    size_t file_contents_size;
    std::vector<segment*> _segments;

    std::map<std::string, object*> _objects;
};
}
