# PrakBS21

log into server:
nc localhost 5678

Commands:
Sets a key to a value:
PUT key val
PUT name1 'Name1'
PUT value1 'Value1'

Retrieves the value for a key:
GET key
GET name1
GET value1

Deletes a key:
DEL key
DEL name1
DEL value1

Starts an exclusive transaction:
BEG

Ends the current transaction:
END

Closes the connection:
QUIT

