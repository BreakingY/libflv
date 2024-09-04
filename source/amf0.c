#include "amf0.h"
#include <inttypes.h>
#include <string.h>
double parseNumber(uint8_t *data, uint32_t data_len, uint32_t *data_used){
    int type = data[0];
    uint64_t number = 0;
    if(type != AMFNUMBER){
        return number;
    }
    data += 1;
    for (int i = 0; i < 8; i++){
        number <<= 8;
        number |= data[i];
    }
    double res;
    memcpy((void *)&res, (void *)&number, sizeof(double));
    *data_used = 9;
    return res;
}
int parseBoolean(uint8_t *data, uint32_t data_len, uint32_t *data_used){
    int type = data[0];
    int boolean = 0;
    if(type != AMFBOOLEAN){
        return boolean;
    }
    boolean = data[1];
    *data_used = 2;
    return boolean;
}
uint8_t* parseString(uint8_t *data, uint32_t data_len, uint32_t *data_used, uint16_t *str_len){
    int type = data[0];
    if(type != AMFSTRING){
        return NULL;
    }
    *str_len = (data[1] << 8) | data[2];
    *data_used = 3 + *str_len;
    return data + 3;
}
uint8_t* parseLongString(uint8_t *data, uint32_t data_len, uint32_t *data_used, uint32_t *str_len){
    int type = data[0];
    if(type != AMFLONGSTRING){
        return NULL;
    }
    *str_len = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
    *data_used = 5 + *str_len;
    return data + 5;
}
AMFDict parseObject(uint8_t *data, uint32_t data_len, uint32_t *data_used){
    int type = data[0];
    AMFDict amf_dict;
    amf_dict.key_value_len = 0;
    if(type != AMFOBJECT){
        return amf_dict;
    }
    int pos = 1;
    while(1){
        uint32_t data_used_tmp;
        uint16_t str_len;
        uint16_t key_len = (data[pos++] << 8) | data[pos++];
        if(key_len == 0){
            pos++; // AMFENDOFOBJECT
            break;
        }
        KV *k_v = &amf_dict.key_value[amf_dict.key_value_len++];
        k_v->key = &data[pos];
        k_v->key_len = key_len;
        pos += key_len;
        int type_value = data[pos];
        switch (type_value)
        {
            case AMFNUMBER:
                k_v->value_type = AMFNUMBER;
                k_v->value.number = parseNumber(data + pos, data_len - pos, &data_used_tmp);
                pos += data_used_tmp;
                break;
            case AMFBOOLEAN:
                k_v->value_type = AMFBOOLEAN;
                k_v->value.boolean = parseBoolean(data + pos, data_len - pos, &data_used_tmp);
                pos += data_used_tmp;
                break;
            case AMFSTRING:
                k_v->value_type = AMFSTRING;
                k_v->value.str = parseString(data + pos, data_len - pos, &data_used_tmp, &str_len);
                k_v->str_len = str_len;
                pos += data_used_tmp;
                break;
            case AMFLONGSTRING:
                k_v->value_type = AMFLONGSTRING;
                k_v->value.long_str = parseLongString(data + pos, data_len - pos, &data_used_tmp, &k_v->str_len);
                pos += data_used_tmp;
                break;
            default:
                break;
        }
    }
    *data_used = pos;
    return amf_dict;
}
AMFDict parseMixedArray(uint8_t *data, uint32_t data_len, uint32_t *data_used){
    int type = data[0];
    AMFDict amf_dict;
    amf_dict.key_value_len = 0;
    if(type != AMFMIXEDARRAY){
        return amf_dict;
    }
    int pos = 1;
    int array_num = (data[pos++] << 24) | (data[pos++] << 16) | (data[pos++] << 8) | data[pos++];
    for(int i = 0; i < array_num; i++){
        uint32_t data_used_tmp;
        uint16_t str_len;
        uint16_t key_len = (data[pos++] << 8) | data[pos++];
        KV *k_v = &amf_dict.key_value[amf_dict.key_value_len++];
        k_v->key = &data[pos];
        k_v->key_len = key_len;
        pos += key_len;
        int type_value = data[pos];
        switch (type_value)
        {
            case AMFNUMBER:
                k_v->value_type = AMFNUMBER;
                k_v->value.number = parseNumber(data + pos, data_len - pos, &data_used_tmp);
                pos += data_used_tmp;
                break;
            case AMFBOOLEAN:
                k_v->value_type = AMFBOOLEAN;
                k_v->value.boolean = parseBoolean(data + pos, data_len - pos, &data_used_tmp);
                pos += data_used_tmp;
                break;
            case AMFSTRING:
                k_v->value_type = AMFSTRING;
                k_v->value.str = parseString(data + pos, data_len - pos, &data_used_tmp, &str_len);
                k_v->str_len = str_len;
                pos += data_used_tmp;
                break;
            case AMFLONGSTRING:
                k_v->value_type = AMFLONGSTRING;
                k_v->value.long_str = parseLongString(data + pos, data_len - pos, &data_used_tmp, &k_v->str_len);
                pos += data_used_tmp;
                break;
            default:
                break;
        }
    }
    *data_used = pos + 3; // AMFENDOFOBJECT
    return amf_dict;
}
void printAMFDict(AMFDict dict){
    for(int i = 0; i < dict.key_value_len; i++){
        KV *k_v = &dict.key_value[i];
        printf("key:%*s ", k_v->key_len, k_v->key);
        switch (k_v->value_type)
        {
            case AMFNUMBER:
                printf("value(number):%lf\n", (double)k_v->value.number);
                break;
            case AMFBOOLEAN:
                printf("value(boolean):%d\n", k_v->value.boolean);
                break;
            case AMFSTRING:
                printf("value(string):%*s\n", k_v->str_len, k_v->value.str);
                break;
            case AMFLONGSTRING:
                printf("value(longstring):%*s\n", k_v->str_len, k_v->value.long_str);
                break;
            default:
                break;
        }
    }
    return;
}
int generateNumber(double number, uint8_t *data, uint32_t data_len){
    if (data_len < 9) {
        return 0;
    }

    data[0] = AMFNUMBER;
    uint64_t numberBits;
    memcpy(&numberBits, &number, sizeof(double));

    for (int i = 0; i < 8; i++) {
        data[1 + i] = (numberBits >> (56 - 8 * i)) & 0xFF;
    }
    return 9;
}

int generaBoolean(int boolean, uint8_t *data, uint32_t data_len){
    if(data_len < 2){
        return 0;
    }
    data[0] = AMFBOOLEAN;
    data[1] = boolean == 0 ? 0 : 1;
    return 2;
}
int generaString(uint8_t *str, uint16_t str_len, uint8_t *data, uint16_t data_len){
    if(data_len < 3 + str_len){
        return 0;
    }
    data[0] = AMFSTRING;
    data[1] = (str_len >> 8) & 0xff;
    data[2] = str_len & 0xff;
    memcpy(data + 3, str, str_len);
    return 3 + str_len;
}
int generaLongString(uint8_t *str, uint32_t str_len, uint8_t *data, uint32_t data_len){
    if(data_len < 5 + str_len){
        return 0;
    }
    data[0] = AMFLONGSTRING;
    data[1] = (str_len >> 24) & 0xff;
    data[2] = (str_len >> 16) & 0xff;
    data[3] = (str_len >> 8) & 0xff;
    data[4] = str_len & 0xff;
    memcpy(data + 5, str, str_len);
    return 5 + str_len;
}
int generateObject(AMFDict dict, uint8_t *data, uint32_t data_len){
    data[0] = AMFOBJECT;
    int pos = 1;
    for(int i = 0; i < dict.key_value_len; i++){
        KV *k_v = &dict.key_value[i];
        data[pos++] = (k_v->key_len >> 8) & 0xff;
        data[pos++] = k_v->key_len & 0xff;
        memcpy(data + pos, k_v->key, k_v->key_len);
        pos += k_v->key_len;
        switch (k_v->value_type)
        {
            case AMFNUMBER:
                pos += generateNumber(k_v->value.number, data + pos, data_len - pos);
                break;
            case AMFBOOLEAN:
                pos += generaBoolean(k_v->value.boolean, data + pos, data_len - pos);
                break;
            case AMFSTRING:
                pos += generaString(k_v->value.str, (uint16_t)(k_v->str_len), data + pos, data_len - pos);
                break;
            case AMFLONGSTRING:
                pos += generaLongString(k_v->value.long_str, k_v->str_len, data + pos, data_len - pos);
                break;
            default:
                break;
        }
    }
    data[pos++] = 0;
    data[pos++] = 0;
    data[pos++] = AMFENDOFOBJECT;
    return pos;
}
int generateMixedArray(AMFDict dict, uint8_t *data, uint32_t data_len){
    data[0] = AMFMIXEDARRAY;
    data[1] = (dict.key_value_len >> 24) & 0xff;
    data[2] = (dict.key_value_len >> 16) & 0xff;
    data[3] = (dict.key_value_len >> 8) & 0xff;
    data[4] = dict.key_value_len & 0xff;
    int pos = 5;
    for(int i = 0; i < dict.key_value_len; i++){
        KV *k_v = &dict.key_value[i];
        data[pos++] = (k_v->key_len >> 8) & 0xff;
        data[pos++] = k_v->key_len & 0xff;
        memcpy(data + pos, k_v->key, k_v->key_len);
        pos += k_v->key_len;
        switch (k_v->value_type)
        {
            case AMFNUMBER:
                pos += generateNumber(k_v->value.number, data + pos, data_len - pos);
                break;
            case AMFBOOLEAN:
                pos += generaBoolean(k_v->value.boolean, data + pos, data_len - pos);
                break;
            case AMFSTRING:
                pos += generaString(k_v->value.str, (uint16_t)(k_v->str_len), data + pos, data_len - pos);
                break;
            case AMFLONGSTRING:
                pos += generaLongString(k_v->value.long_str, k_v->str_len, data + pos, data_len - pos);
                break;
            default:
                break;
        }
    }
    data[pos++] = 0;
    data[pos++] = 0;
    data[pos++] = AMFENDOFOBJECT;
    return pos;
}
void setAMFDict(AMFDict *dict, int type, uint8_t *key, uint16_t key_len, double number, int boolean, char *str, char *long_str, uint32_t str_len){
    if(dict->key_value_len >= sizeof(dict->key_value)){
        return;
    }
    KV *k_v = &dict->key_value[dict->key_value_len++];
    k_v->key = key;
    k_v->key_len = key_len;
    switch (type)
    {
        case AMFNUMBER:
            k_v->value_type = AMFNUMBER;
            k_v->value.number = number;
            break;
        case AMFBOOLEAN:
            k_v->value_type = AMFBOOLEAN;
            k_v->value.number = boolean;
            break;
        case AMFSTRING:
            k_v->value_type = AMFSTRING;
            k_v->value.str = str;
            k_v->str_len = str_len;
            break;
        case AMFLONGSTRING:
            k_v->value_type = AMFLONGSTRING;
            k_v->value.long_str = long_str;
            k_v->str_len = str_len;
            break;
        default:
            break;
    }
    return;
}