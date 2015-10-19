
include "dsn.thrift"

namespace cpp dsn.apps

service repis
{
    dsn.blob read(1:dsn.blob request);
    dsn.blob write(1:dsn.blob request);
}
