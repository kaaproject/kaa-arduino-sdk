import base64

filename = 'root_ca.pem'
pem_cer_arr_base64 = bytearray()

with open(filename, 'rb') as pem_cer_file:
    for line in pem_cer_file:
        if line == b'-----BEGIN CERTIFICATE-----\r\n':
            continue
        if line == b'-----END CERTIFICATE-----\r\n':
            break
        line = line.rstrip()
        pem_cer_arr_base64 += line

pem_cer_hex_arr = base64.b64decode(pem_cer_arr_base64)

print('// '+filename)
print('const unsigned char caCert[] PROGMEM = {\n')

outString = ''
caCertLen = 0
i=0

for x in pem_cer_hex_arr:
    outString += f"{x:#0{4}x}" + ', '
    caCertLen = caCertLen + 1
    i=i+1

outString = outString[:-2]

print(outString+'};\n')
print('const unsigned int caCertLen = ' + str(caCertLen) + ';')