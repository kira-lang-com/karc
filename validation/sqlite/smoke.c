#include "sqlite3.c"

int main() {
    sqlite3* db = 0;
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) { return 1; }
    int rc = sqlite3_exec(db,
        "CREATE TABLE numbers(value INTEGER);"
        "INSERT INTO numbers VALUES(19),(23);", 0, 0, 0);
    if (rc != SQLITE_OK) { sqlite3_close(db); return 2; }
    sqlite3_stmt* statement = 0;
    rc = sqlite3_prepare_v2(db, "SELECT sum(value) FROM numbers", -1, &statement, 0);
    if (rc != SQLITE_OK) { sqlite3_close(db); return 3; }
    rc = sqlite3_step(statement);
    int result = rc == SQLITE_ROW && sqlite3_column_int(statement, 0) == 42 ? 0 : 4;
    if (sqlite3_finalize(statement) != SQLITE_OK) { result = 5; }
    if (sqlite3_close(db) != SQLITE_OK) { result = 6; }
    return result;
}
