

fid = open('icons.txt')

      #{
        #'file'" 'images/A10.png',
        #'name'" 'ACTION_MOON',
        #'type'" 'png'
      #},

entries = []      
for line in fid.read().split('\n'):
    line = line.strip()
    if len(line) == 0:
      continue
    
    resource_name = line.split(".")[0]
    resource_name = resource_name.upper()
    
    entry = '   {\n'
    entry+= '      "file" : "images/%s",\n' % line
    entry+= '      "name" : "%s",\n' % resource_name
    entry+= '      "type" : "png"\n'
    entry+= '   }'
    entries.append(entry)


print ',\n'.join(entries)
print '\n'
