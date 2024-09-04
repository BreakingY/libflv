#ifndef _AMF0_H_
#define _AMF0_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define AMFNUMBER	    0x00
#define AMFBOOLEAN	    0x01
#define AMFSTRING	    0x02
#define AMFOBJECT	    0x03
#define AMFMOVIECLIP	0x04
#define AMFNULL	        0x05
#define AMFUNDEFINED	0x06
#define AMFREFERENCE	0x07
#define AMFMIXEDARRAY	0x08
#define AMFENDOFOBJECT	0x09
#define AMFARRAY	    0x0a
#define AMFDATA	        0x0b
#define AMFLONGSTRING	0x0c
#define AMFUNSUPPORTED	0x0d
#define AMFRECORDSET	0x0e
#define AMFXML	        0x0f
#define AMFTYPEDOBJECT	0x10
#define AMF3DATA	0x11
union AMFValue{
    double number;
    int boolean;
    char *str;
    char *long_str;
};

typedef struct KVSt{
    uint8_t *key;
    uint32_t key_len;
    union AMFValue value;
    uint32_t str_len;
    int value_type;
}KV;
typedef struct AMFDictSt{
    KV key_value[1024];
    uint32_t key_value_len;
}AMFDict;
double parseNumber(uint8_t *data, uint32_t data_len, uint32_t *data_used);
int parseBoolean(uint8_t *data, uint32_t data_len, uint32_t *data_used);
uint8_t* parseString(uint8_t *data, uint32_t data_len, uint32_t *data_used, uint16_t *str_len);
uint8_t* parseLongString(uint8_t *data, uint32_t data_len, uint32_t *data_used, uint32_t *str_len);
AMFDict parseObject(uint8_t *data, uint32_t data_len, uint32_t *data_used);
AMFDict parseMixedArray(uint8_t *data, uint32_t data_len, uint32_t *data_used);
void printAMFDict(AMFDict dict);

int generateNumber(double number, uint8_t *data, uint32_t data_len);
int generaBoolean(int boolean, uint8_t *data, uint32_t data_len);
int generaString(uint8_t *str, uint16_t str_len, uint8_t *data, uint16_t data_len);
int generaLongString(uint8_t *str, uint32_t str_len, uint8_t *data, uint32_t data_len);
int generateObject(AMFDict dict, uint8_t *data, uint32_t data_len);
int generateMixedArray(AMFDict dict, uint8_t *data, uint32_t data_len);

void setAMFDict(AMFDict *dict, int type, uint8_t *key, uint16_t key_len, double number, int boolean, char *str, char *long_str, uint32_t str_len);

#endif