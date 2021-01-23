-- Create table and load data to MySQL: https://github.com/catarinaribeir0/queries-tpch-dbgen-mysql/blob/master/README.md

ALTER TABLE customer ADD q3_annot int(11);
ALTER TABLE customer ADD q10_annot int(11);
ALTER TABLE customer ADD q18_annot int(11);
ALTER TABLE customer ADD q8_annot int(11);

ALTER TABLE lineitem ADD q3_annot int(11);
ALTER TABLE lineitem ADD q10_annot int(11);
ALTER TABLE lineitem ADD q18_annot int(11);
ALTER TABLE lineitem ADD q8_annot int(11);
ALTER TABLE lineitem ADD q9_annot1 int(11);
ALTER TABLE lineitem ADD q9_annot2 int(11);

ALTER TABLE orders ADD o_year int(11);
ALTER TABLE orders ADD q3_annot int(11);
ALTER TABLE orders ADD q10_annot int(11);
ALTER TABLE orders ADD q18_annot int(11);
ALTER TABLE orders ADD q8_annot int(11);
ALTER TABLE orders ADD q9_annot int(11);

ALTER TABLE part ADD q8_annot int(11);
ALTER TABLE part ADD q9_annot int(11);

ALTER TABLE partsupp ADD q9_annot1 int(11);
ALTER TABLE partsupp ADD q9_annot2 int(11);

ALTER TABLE supplier ADD q8_annot1 int(11);
ALTER TABLE supplier ADD q8_annot2 int(11);

ALTER TABLE supplier ADD q9_annot0 int(11);
ALTER TABLE supplier ADD q9_annot1 int(11);
ALTER TABLE supplier ADD q9_annot2 int(11);
ALTER TABLE supplier ADD q9_annot3 int(11);
ALTER TABLE supplier ADD q9_annot4 int(11);
ALTER TABLE supplier ADD q9_annot5 int(11);
ALTER TABLE supplier ADD q9_annot6 int(11);
ALTER TABLE supplier ADD q9_annot7 int(11);
ALTER TABLE supplier ADD q9_annot8 int(11);
ALTER TABLE supplier ADD q9_annot9 int(11);
ALTER TABLE supplier ADD q9_annot10 int(11);
ALTER TABLE supplier ADD q9_annot11 int(11);
ALTER TABLE supplier ADD q9_annot12 int(11);
ALTER TABLE supplier ADD q9_annot13 int(11);
ALTER TABLE supplier ADD q9_annot14 int(11);
ALTER TABLE supplier ADD q9_annot15 int(11);
ALTER TABLE supplier ADD q9_annot16 int(11);
ALTER TABLE supplier ADD q9_annot17 int(11);
ALTER TABLE supplier ADD q9_annot18 int(11);
ALTER TABLE supplier ADD q9_annot19 int(11);
ALTER TABLE supplier ADD q9_annot20 int(11);
ALTER TABLE supplier ADD q9_annot21 int(11);
ALTER TABLE supplier ADD q9_annot22 int(11);
ALTER TABLE supplier ADD q9_annot23 int(11);
ALTER TABLE supplier ADD q9_annot24 int(11);

-- batch 
UPDATE customer SET 
    q3_annot=CAST(c_mktsegment = 'AUTOMOBILE' AS SIGNED INTEGER), 
    q10_annot=1, 
    q18_annot=1, 
    q8_annot=CAST(c_nationkey in (8,9,12,18,21) AS SIGNED INTEGER);

UPDATE orders SET
    o_year=extract(year FROM o_orderdate),
    q3_annot=CAST(o_orderdate < date '1995-03-13' AS SIGNED INTEGER),
    q10_annot=CAST(o_orderdate >= date '1993-08-01' AND o_orderdate < date '1993-11-01' AS SIGNED INTEGER),
    q18_annot=1,
    q8_annot=CAST(o_orderdate between date '1995-01-01' and date '1996-12-31' AS SIGNED INTEGER),
    q9_annot=1;

UPDATE lineitem SET 
    q3_annot=CAST(l_shipdate > date '1995-03-13' AS SIGNED INTEGER)*CAST(l_extendedprice * (1 - l_discount) AS SIGNED INTEGER),
    q10_annot=CAST(l_returnflag = 'R' AS SIGNED INTEGER)*CAST(l_extendedprice * (1 - l_discount) AS SIGNED INTEGER),
    q18_annot=0,
    q8_annot=CAST(l_extendedprice * (1 - l_discount) AS SIGNED INTEGER),
    q9_annot1=CAST(l_extendedprice * (1 - l_discount) AS SIGNED INTEGER),
    q9_annot2=CAST(l_quantity AS SIGNED INTEGER);

UPDATE part SET
    q8_annot=CAST(p_type = 'SMALL PLATED COPPER' AS SIGNED INTEGER),
    q9_annot=CAST(p_name like '%green%' AS SIGNED INTEGER);

UPDATE partsupp SET
    q9_annot1=1,
    q9_annot2=ps_supplycost;

-- batch
UPDATE lineitem line1 SET line1.q18_annot = line1.l_quantity WHERE line1.l_orderkey IN 
    (SELECT line2.l_orderkey FROM (SELECT l_orderkey,sum(l_quantity) FROM lineitem GROUP BY l_orderkey HAVING sum(l_quantity)>300) line2);

UPDATE supplier SET
    q8_annot1=CAST(s_nationkey=8 AS SIGNED INTEGER),
    q8_annot2=1,
    q9_annot0=CAST(s_nationkey=0 AS SIGNED INTEGER),
    q9_annot1=CAST(s_nationkey=1 AS SIGNED INTEGER),
    q9_annot2=CAST(s_nationkey=2 AS SIGNED INTEGER),
    q9_annot3=CAST(s_nationkey=3 AS SIGNED INTEGER),
    q9_annot4=CAST(s_nationkey=4 AS SIGNED INTEGER),
    q9_annot5=CAST(s_nationkey=5 AS SIGNED INTEGER),
    q9_annot6=CAST(s_nationkey=6 AS SIGNED INTEGER),
    q9_annot7=CAST(s_nationkey=7 AS SIGNED INTEGER),
    q9_annot8=CAST(s_nationkey=8 AS SIGNED INTEGER),
    q9_annot9=CAST(s_nationkey=9 AS SIGNED INTEGER),
    q9_annot10=CAST(s_nationkey=10 AS SIGNED INTEGER),
    q9_annot11=CAST(s_nationkey=11 AS SIGNED INTEGER),
    q9_annot12=CAST(s_nationkey=12 AS SIGNED INTEGER),
    q9_annot13=CAST(s_nationkey=13 AS SIGNED INTEGER),
    q9_annot14=CAST(s_nationkey=14 AS SIGNED INTEGER),
    q9_annot15=CAST(s_nationkey=15 AS SIGNED INTEGER),
    q9_annot16=CAST(s_nationkey=16 AS SIGNED INTEGER),
    q9_annot17=CAST(s_nationkey=17 AS SIGNED INTEGER),
    q9_annot18=CAST(s_nationkey=18 AS SIGNED INTEGER),
    q9_annot19=CAST(s_nationkey=19 AS SIGNED INTEGER),
    q9_annot20=CAST(s_nationkey=20 AS SIGNED INTEGER),
    q9_annot21=CAST(s_nationkey=21 AS SIGNED INTEGER),
    q9_annot22=CAST(s_nationkey=22 AS SIGNED INTEGER),
    q9_annot23=CAST(s_nationkey=23 AS SIGNED INTEGER),
    q9_annot24=CAST(s_nationkey=24 AS SIGNED INTEGER);

-- batch
-- Optional. Just want to reduce time of exporting files.

ALTER TABLE `customer`
  DROP `C_ADDRESS`,
  DROP `C_PHONE`,
  DROP `C_ACCTBAL`,
  DROP `C_MKTSEGMENT`,
  DROP `C_COMMENT`;

ALTER TABLE `lineitem`
  DROP `L_QUANTITY`,
  DROP `L_EXTENDEDPRICE`,
  DROP `L_DISCOUNT`,
  DROP `L_TAX`,
  DROP `L_RETURNFLAG`,
  DROP `L_LINESTATUS`,
  DROP `L_SHIPDATE`,
  DROP `L_COMMITDATE`,
  DROP `L_RECEIPTDATE`,
  DROP `L_SHIPINSTRUCT`,
  DROP `L_SHIPMODE`,
  DROP `L_COMMENT`;

ALTER TABLE `orders`
  DROP `O_ORDERSTATUS`,
  DROP `O_ORDERPRIORITY`,
  DROP `O_CLERK`,
  DROP `O_COMMENT`;

ALTER TABLE `part`
  DROP `P_NAME`,
  DROP `P_MFGR`,
  DROP `P_BRAND`,
  DROP `P_TYPE`,
  DROP `P_SIZE`,
  DROP `P_CONTAINER`,
  DROP `P_RETAILPRICE`,
  DROP `P_COMMENT`;

ALTER TABLE `supplier`
  DROP `S_NAME`,
  DROP `S_ADDRESS`,
  DROP `S_PHONE`,
  DROP `S_ACCTBAL`,
  DROP `S_COMMENT`;

ALTER TABLE `partsupp`
  DROP `PS_AVAILQTY`,
  DROP `PS_COMMENT`;
