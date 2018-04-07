
import sys

if len(sys.argv) < 2:
    print "Usage: ", sys.argv[0], '<input_file>'
    exit(1)

data = open(sys.argv[1], 'rb').read()

strdata = 'static const char ' + sys.argv[1].replace('.','_') + '[] = \n\t"'
llen = 1
for d in data:
    strdata += '\\x%.02x' % ord(d)
    llen += 4
    if llen >= 160:
        strdata += '"\n\t"'
        llen = 1
strdata += '";\n'
        
outfile = open(sys.argv[1] + '.c', 'wb')
outfile.write(strdata)
outfile.close()


