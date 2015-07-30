#include <iostream> // Debugging
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <map>
#include <algorithm>
#include <string>

#include "tdms.hpp"
#include "tdms_impl.hpp"
#include "log.hpp"
#include "data_extraction.hpp"

namespace TDMS
{

const std::map<const std::string, int32_t> segment::_toc_properties =
{
    {"kTocMetaData", int32_t(1) << 1},
    {"kTocRawData", int32_t(1) << 3},
    {"kTocDAQmxRawData", int32_t(1) << 7},
    {"kTocInterleavedData", int32_t(1) << 5},
    {"kTocBigEndian", int32_t(1) << 6},
    {"kTocNewObjList", int32_t(1) << 2}
};

template<typename T>
inline std::function<void (const unsigned char*, void*)> put_on_heap_generator(std::function<T (const unsigned char*)> f)
{
    return [f](const unsigned char* data, void* ptr)
    {
        *((T*)ptr) = f(data);
    };
}

template<typename T>
inline std::function<void (const unsigned char*, void*)> put_le_on_heap_generator()
{
    return put_on_heap_generator<T>(&read_le<T>);
}

template<typename T>
inline std::function<void (const unsigned char*, void*, size_t)> copy_array_reader_generator()
{
    return [](const unsigned char* source, void* tgt, size_t number_values){
        memcpy(tgt, source, number_values*sizeof(T));
    };
}

std::function<void (const unsigned char*, void*)> not_implemented = [](const unsigned char*, void*){throw std::runtime_error{"Reading this type is not implemented. Aborting"};};

const std::map<uint32_t, const data_type_t> data_type_t::_tds_datatypes = {
    {         0, data_type_t("tdsTypeVoid", 0, not_implemented)},
    {         1, data_type_t("tdsTypeI8",  1, put_le_on_heap_generator<int8_t>(), copy_array_reader_generator<int8_t>())},
    {         2, data_type_t("tdsTypeI16", 2, put_le_on_heap_generator<int16_t>())},
    {         3, data_type_t("tdsTypeI32", 4, put_le_on_heap_generator<int32_t>())},
    {         4, data_type_t("tdsTypeI64", 8, put_le_on_heap_generator<int32_t>())},
    {         5, data_type_t("tdsTypeU8",  1, put_le_on_heap_generator<uint8_t>())},
    {         6, data_type_t("tdsTypeU16", 2, put_le_on_heap_generator<uint16_t>())},
    {         7, data_type_t("tdsTypeU32", 4, put_le_on_heap_generator<uint32_t>())},
    {         8, data_type_t("tdsTypeU64", 8, put_le_on_heap_generator<uint64_t>())},
    {         9, data_type_t("tdsTypeSingleFloat", 4, put_on_heap_generator<float>(&read_le_float), copy_array_reader_generator<float>())},
    {        10, data_type_t("tdsTypeDoubleFloat", 8, put_on_heap_generator<double>(&read_le_double), copy_array_reader_generator<double>())},
    {        11, data_type_t("tdsTypeExtendedFloat", 0,not_implemented)},
    {        12, data_type_t("tdsTypeDoubleFloatWithUnit", 8, not_implemented)},
    {        13, data_type_t("tdsTypeExtendedFloatWithUnit", 0, not_implemented)},
    {      0x19, data_type_t("tdsTypeSingleFloatWithUnit", 4, not_implemented)},
    {      0x20, data_type_t("tdsTypeString", 0, not_implemented)},
    {      0x21, data_type_t("tdsTypeBoolean", 1, not_implemented)},
    {      0x44, data_type_t("tdsTypeTimeStamp", 16, put_on_heap_generator<time_t>(&read_timestamp))},
    {0xFFFFFFFF, data_type_t("tdsTypeDAQmxRawData", 0, not_implemented)}
};


segment::segment(const unsigned char* contents, 
        segment* previous_segment,
        file* file)
    : _parent_file(file)
{
    const char* header = "TDSm";
    if(memcmp(contents, header, 4) != 0)
    {
        throw segment::no_segment_error();
    }
    contents += 4;

    // First four bytes are toc mask
    int32_t toc_mask = read_le<int32_t>(contents);
    
    for(auto prop : segment::_toc_properties)
    {
        _toc[prop.first] = (toc_mask & prop.second) != 0;
        log::debug << "Property " << prop.first << " is " 
             << _toc[prop.first] << log::endl;
    }
    contents += 4;
    
    // Four bytes for version number
    int32_t version = read_le<int32_t>(contents);
    log::debug << "Version: " << version << log::endl;
    switch (version)
    {
    case 4712:
    case 4713:
        break;
    default:
        std::cerr << "segment: unknown version number " << version << std::endl;
        break;
    }
    contents += 4;
    
    // 64 bits pointer to next segment
    // and same for raw data offset
    uint64_t next_segment_offset = read_le<uint64_t>(contents);
    contents += 8;
    uint64_t raw_data_offset = read_le<uint64_t>(contents);
    contents += 8;

    this->_data = contents + raw_data_offset; // Remember location of the data

    if(next_segment_offset == 0xFFFFFFFFFFFFFFFF) // That's 8 times FF, or 16 F's, aka
                                                  // the maximum unsigned int64_t.
    {
        throw std::runtime_error("Labview probably crashed, file is corrupt. Not attempting to read.");
    }
    this->_next_segment_offset = next_segment_offset + 7*4;
    this->_raw_data_offset = raw_data_offset + 7*4;

    _parse_metadata(contents, previous_segment);
}

void segment::_parse_metadata(const unsigned char* data, 
        segment* previous_segment)
{
    if(!this->_toc["kTocMetaData"])
    {
        if(previous_segment == nullptr)
            throw std::runtime_error("kTocMetaData is set for segment, but"
                    "there is no previous segment.");
        this->_ordered_objects = previous_segment->_ordered_objects;
        _calculate_chunks();
        return;
    }
    if(!this->_toc["kTocNewObjList"])
    {
        // In this case, there can be a list of new objects that
        // are appended, or previous objects can also be repeated
        // if their properties change
        
        if(previous_segment == nullptr)
            throw std::runtime_error("kTocNewObjList is set for segment, but"
                    "there is no previous segment.");
        this->_ordered_objects = previous_segment->_ordered_objects;
    }

    // Read number of metadata objects
    int32_t num_objs = read_le<int32_t>(data);
    data += 4;

    for(size_t i = 0; i < num_objs; ++i)
    {
        std::string object_path = read_string(data);
        data += 4 + object_path.size();
        log::debug << object_path << log::endl;

        TDMS::object* obj = nullptr;
        if(_parent_file->_objects.find(object_path) 
                != _parent_file->_objects.end())
        {
            obj = _parent_file->_objects[object_path];
        }
        else
        {
            obj = new TDMS::object(object_path);
            _parent_file->_objects[object_path] = obj;
        }
        bool updating_existing = false;

        std::shared_ptr<segment::object> segment_object(nullptr);

        if(!_toc["kTocNewObjList"])
        {
            // Search for the same object from the previous
            // segment object list
            auto it = std::find_if(this->_ordered_objects.begin(),
                    this->_ordered_objects.end(),
                    [obj](const std::shared_ptr<segment::object> o){
                        return (o->_tdms_object == obj);
                        // TODO: compare by value?
                        //       define an operator==() ?
                    });
            if(it != this->_ordered_objects.end())
            {
                updating_existing = true;
                log::debug << "Updating object in segment list." << log::endl;
                segment_object = *it;
            }
        }
        if(!updating_existing)
        {
            if(obj->_previous_segment_object != nullptr)
            {
                log::debug << "Copying previous segment object" << log::endl;
                segment_object = std::make_shared<segment::object>(*obj->_previous_segment_object);
            }
            else
            {
                segment_object = std::shared_ptr<segment::object>(new segment::object(obj));
            }
            this->_ordered_objects.push_back(segment_object);
        }
        data = segment_object->_parse_metadata(data);
        obj->_previous_segment_object = segment_object;
    }
    _calculate_chunks();
}
void segment::_calculate_chunks()
{
    // Work out the number of chunks the data is in, for cases
    // where the meta data doesn't change at all so there is no
    // lead in.
    // Also increments the number of values for objects in this
    // segment, based on the number of chunks.
    
    // Count the datasize
    long long data_size = 0;
    std::for_each(
            _ordered_objects.begin(), 
            _ordered_objects.end(), 
            [&data_size](std::shared_ptr<segment_object> o)
            {
                if(o->_has_data)
                {
                    data_size += o->_data_size;
                }
            }
        );
    long long total_data_size = this->_next_segment_offset - this->_raw_data_offset;

    if(data_size < 0 || total_data_size < 0)
    {
        throw std::runtime_error("Negative data size");
    }
    else if(data_size == 0)
    {
        if(total_data_size != data_size)
        {
            throw std::runtime_error("Zero channel data size but non-zero data "
                "length based on segment offset.");
        }
        this->_num_chunks = 0;
        return;
    }
    if ((total_data_size % data_size) != 0)
    {
        throw std::runtime_error("Data size is not a multiple of the "
                                    "chunk size");
    }
    else
    {
        this->_num_chunks = total_data_size / data_size;
    }

    // Update data count for the overall tdms object
    // using the data count for this segment.
    for(auto obj: this->_ordered_objects)
    {
        if(obj->_has_data)
        {
            obj->_tdms_object->_number_values 
                += (obj->_number_values * this->_num_chunks);
        }
    }
}

void segment::_parse_raw_data()
{
    if(!this->_toc["kTocRawData"])
        return;
    size_t total_data_size = _next_segment_offset - _raw_data_offset;

    endianness e = LITTLE;
    const unsigned char* d = _data;

    if(this->_toc["kTocBigEndian"])
    {
        throw std::runtime_error("Big endian reading not yet implemented");
        e = BIG;
    }
    for(size_t chunk = 0; chunk < _num_chunks; ++chunk)
    {
        if(this->_toc["kTocInterleavedData"])
        {
            log::debug << "Data is interleaved" << log::endl;
            throw std::runtime_error("Reading inteleaved data not supported yet");
        }
        else
        {
            log::debug << "Data is contiguous" << log::endl;
            for(auto obj : _ordered_objects)
            {
                if(obj->_has_data)
                {
                    obj->_read_values(d, e);
                }
            }
        }
    }
}

void segment_object::_read_values(const unsigned char*& data, endianness e)
{
    if(_data_type.name == "tdsTypeString")
    {
        log::debug << "Reading string data" << log::endl;
        throw std::runtime_error("Reading string data not yet implemented");
        // TODO ^
    }
    else
    {
        unsigned char* read_data = ((unsigned char*)_tdms_object->_data) + _tdms_object->_data_insert_position;

        _data_type.read_array_to(data, read_data, _number_values);

        _tdms_object->_data_insert_position += (_number_values*_data_type.ctype_length);
        data += (_number_values*_data_type.ctype_length);
    }
}

segment::~segment()
{
}


segment_object::segment_object(object* o)
    : _tdms_object(o),
      _data_type(data_type_t::_tds_datatypes.at(0))
{
    _number_values = 0;
    _data_size = 0;
    _has_data = true;
    //_data_type = None;
    //_dimension = 1;
}

const unsigned char* segment_object::_parse_metadata(const unsigned char* data)
{
    // Read object metadata and update object information
    uint32_t raw_data_index = read_le<uint32_t>(data);
    data += 4;

    log::debug << "Reading metadata for object " << _tdms_object->_path << log::endl
        << "raw_data_index: " << raw_data_index << log::endl;

    if(raw_data_index == 0xFFFFFFFF)
    {
        log::debug << "Object has no data" << log::endl;
        _has_data = false;
    }
    else if(raw_data_index == 0x00000000)
    {
        log::debug << "Object has same data structure "
            "as in the previous segment" << log::endl;
        _has_data = true;
    }
    else
    {
        // raw_data_index gives the length of the index information.
        _tdms_object->_has_data = _has_data = true;
        // Read the datatype
        uint32_t datatype = read_le<uint32_t>(data);
        data += 4;

        try
        {
            _data_type = data_type_t::_tds_datatypes.at(datatype);
        }
        catch(std::out_of_range& e)
        {
            throw std::out_of_range("Unrecognized datatype in file");
        }
        if(_tdms_object->_data_type.is_valid()
                and _tdms_object->_data_type != _data_type)
        {
            throw std::runtime_error("Segment object doesn't have the same data "
                    "type as previous segments");
        }
        else
        {
            _tdms_object->_data_type = _data_type;
        }

        log::debug << "datatype " << _data_type.name << log::endl;

        // Read data dimension
        _dimension = read_le<uint32_t>(data);
        data += 4;
        if(_dimension != 1)
            log::debug << "Warning: dimension != 0" << log::endl;

        // Read the number of values
        _number_values = read_le<uint64_t>(data);
        data += 8;

        // Variable length datatypes have total length
        if(_data_type.name == "tdsTypeString" /*or None*/)
        {
            _data_size = read_le<uint64_t>(data);
            data += 8;
        }
        else
        {
            _data_size = (_number_values * _dimension * _data_type.length);
        }
        log::debug << "Number of elements in segment: " << _number_values << log::endl;
    }
    // Read data properties
    uint32_t num_properties = read_le<uint32_t>(data);
    data += 4;
    log::debug << "Reading " << num_properties << " properties" << log::endl;
    for(size_t i = 0; i < num_properties; ++i)
    {
        std::string prop_name = read_string(data);
        data += 4 + prop_name.size();
        // Property data type
        auto prop_data_type = data_type_t::_tds_datatypes.at(read_le<uint32_t>(data));
        data += 4;
        if(prop_data_type.name == "tdsTypeString")
        {
            std::string* property = new std::string(read_string(data));
            log::debug << "Property " << prop_name << ": " << *property << log::endl;
            data += 4 + property->size();
            _tdms_object->_properties.emplace(prop_name, 
                std::shared_ptr<object::property>(
                    new object::property(prop_data_type, (void*)property)));
        }
        else
        {
            void* prop_val = prop_data_type.read(data);
            if(prop_val == nullptr)
            {
                throw std::runtime_error("Unsupported datatype " + prop_data_type.name);
            }
            data += prop_data_type.length;
            _tdms_object->_properties.emplace(prop_name, 
                std::shared_ptr<object::property>(
                    new object::property(prop_data_type, prop_val)));
        }
    }

    return data;
}
}
