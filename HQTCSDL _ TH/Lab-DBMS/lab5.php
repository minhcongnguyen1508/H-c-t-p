#C:\xampp\mysql\data\sakila  #file chứa link gốc

#Cau 1:
SELECT rental_id, rental_date, inventory_id, customer_id, ifnull(return_date, ' '), staff_id, last_update 
INTO OUTFILE 'C:/Users/Cong/Desktop/rental.txt'
FIELDS 
	terminated by ','
	ESCAPED BY '*' 
	ENCLOSED BY '"' 
	#dinh nghia khi tach truong
FROM rental;

##############
#drop table rental;
create table rental1(rental_id INT unsigned not null primary key, rental_date DATETIME, inventory_id MEDIUMINT, customer_id SMALLINT, return_date DATETIME, staff_id INT, last_update TIMESTAMP);
load data infile 'C:/Users/Cong/Desktop/rental.txt' 
into table rental1
fields 
	terminated by ','
	ESCAPED BY '*' 
	ENCLOSED BY '"' ;
SET return_date = NULLIF(return_date, ' ');

#2
create table rental_lastest(film_id INT, store_id INT, rental_date DATETIME);
insert into rental_lastest
select i.film_id, i.store_id, r.rental_date 
from rental r
join inventory i on r.inventory_id = i.inventory_id
where month(rental_date) = 8 AND year(rental_date) = 2005;

#############

#3
mysqldump --flush-logs --no-create-info -u root -p sakila> C:/Users/Cong/Desktop/backup-data.sql

mysqldump --flush-logs --no-data -u root -p sakila> C:/Users/Cong/Desktop/dump-struct.sql

#####################
#4
# start sever
mysqld -u root -P 3306 --log-bin --console
#chi dinh cong 3306 
Show binary logs;
 
# server 
mysqld -u root --log-bin --binlog-do-db --console
mysqlbinlog C:/xampp/mysql/data/Cong-PC-bin.000002 > C:/Users/Cong/Desktop/log

update film
set language_id = 1
where film_id = 1;

set session binlog_format = ROW;