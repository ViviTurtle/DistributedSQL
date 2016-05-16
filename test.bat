set echo=on
del dbfile.bin *.tab
cl db.cpp
rem db "create table test (col1 char(10), col2 int, col3 int not null)"
rem db "insert into test values ( 'one', 1, 11 )"
rem db "insert into test values ( 'two', 2, 22 )"
rem db "insert into test values ( 'three', 3, 33 )"
rem db "insert into test values ( 'three', 3, 3 )"
rem db "insert into test values ( NULL, NULL, 3  )"
rem db "insert into test values ( 'test', 4, 3  )"
rem rem db "select * from test"
rem rem rem db "update test SET col1 = NULL where col2 IS NULL"
rem rem db "update test SET col2 = NULL where col3 < 30"
rem rem db "select * from test"
rem rem db "select * from test where col3 = 3 and col2 IS NULL"
rem rem db "select col2, col3 from test where col3 = 3 or col2 IS NULL"
rem rem rem Testing columns
rem rem rem db "select col4 from test where col2 = 3"
rem rem db "select col2, col3 from test where col1 IS NOT NULL"
rem db "select * from test"
rem db "select sum(col2) from test where col1 = 'three'"


