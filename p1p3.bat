rem Project Part 3 test batch file
rem
rem cleanup
rem
del dbfile.bin
del db.lo* *.log *.tab *.obj *.pdb *.ilk first second third
cl db.cpp
rem
rem 01. Setup and inserts
rem
db "create table tab1(name char(16), quizzes int, midterm int, final int)"
db "create table tab2(college char(20), zipcode char(5), rank int)"
db "insert into tab1 values('Siu', 11, 80, 560)"
db "insert into tab1 values('Frank', 22, 100, 700)"
db "insert into tab1 values('Jordon', 33, 75, 525)"
db "insert into tab2 values('UCLA', '11111', 3)"
db "insert into tab2 values('SJSU', '22222', 10)"
db "insert into tab2 values('Stanford', '33333', 2)"
db "select * from tab1"
db "select * from tab2"
rem
rem Check transaction log
rem
type db.log
rem
rem 02. Take backup, check image size and db.log
rem
db "backup to first"
rem
rem **Size** first=584; dbfile.bin=336; tab1=120 (3 rows); tab2=120 (3 rows)
