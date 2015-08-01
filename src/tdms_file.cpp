#include <iostream> // Debugging
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <map>

#include "tdms.hpp"
#include "log.hpp"
#include "tdms_impl.hpp"

namespace TDMS
{

file::file(const std::string& filename)
{
    // Read file into memory
    FILE* f = fopen(filename.c_str(), "r");
    if(!f)
    {
        throw std::runtime_error("File \"" + filename + "\" could not be opened");
    }
    fseek(f, 0, SEEK_END);
    file_contents_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    file_contents = (unsigned char*) malloc(file_contents_size * sizeof(unsigned char*));

    size_t read = fread(file_contents, file_contents_size, 1, f);
    fclose(f);
    if(read != 1)
    {
        throw std::runtime_error("File \"" + filename + "\" could not be read");
    }

    // Now parse the segments
    _parse_segments();

    file_contents_size = 0;
    free(file_contents);
}

void file::_parse_segments()
{
    size_t offset = 0;
    segment* prev = nullptr;
    // First read the metadata of the segments
    while(offset < file_contents_size - 8*4)
    {
        try
        {
            segment* s = new segment(file_contents + offset, prev, this);
            offset += s->_next_segment_offset;
            _segments.push_back(s);
            prev = s;
        }
        catch(segment::no_segment_error& e)
        {
            // Last segment was parsed.
            break;
        }
    }
    for(auto obj: this->_objects)
    {
        obj.second->_initialise_data();
    }
    for(auto seg: this->_segments)
    {
        seg->_parse_raw_data();
    }
}

const object* file::operator[](const std::string& key)
{
    return _objects.at(key);
}

file::~file()
{
    for(segment* _s : _segments)
        delete _s;
    for(auto _o : _objects)
        delete _o.second;
}

void object::_initialise_data()
{
    if(_number_values == 0)
        return;
    size_t s = _number_values * _data_type.ctype_length;
    log::debug << "Assigned " << s << " bytes for object " << _path << "#values" << _number_values << "*type" << _data_type.ctype_length << log::endl;
    _data = malloc(s);
    this->_data_insert_position = 0;
}

object::property::~property()
{
    if(value == nullptr) 
    {
        log::debug << "DOUBLE FREE" << log::endl;
        return;
    }
    if(data_type.name == "tdsTypeString")
    {
        delete (std::string*) value;
    }
    else
    {
        free(value);
    }
    value = nullptr;
}
void data_type_t::_init_default_array_reader()
{
    auto read_to = this->read_to;
    auto ctype_length = this->ctype_length;
    read_array_to = [read_to, ctype_length](const unsigned char* source, void* target, size_t number_values){
        log::debug << "Doing iterative reading" << log::endl;
        for(size_t i = 0; i < number_values; ++i)
        {
            read_to(source + (i*ctype_length), (void*)(((char*)target) + (i*ctype_length)));
        }
    };
}
}
