import sys

a = sys.argv[1]

if(a == 'peloton'):
	with open('../dml/peloton_script625000.sql', 'a') as the_file:
		the_file.write("DROP TABLE IF EXISTS UDF_TEST;\n")
		the_file.write("CREATE TABLE UDF_TEST(a double);\n")

		for i in range(0, 625000):
			cmd = 'insert into UDF_TEST values(%s);\n' % (str(i*1.0))
			the_file.write(cmd)

		the_file.write("CREATE OR REPLACE FUNCTION increment(i double) RETURNS double AS $$ BEGIN RETURN i + 1; END; $$ LANGUAGE plpgsql;\n")
		the_file.write("select count(*) from UDF_TEST;\n");
else:
	with open('../dml/postgres_script625000.sql', 'a') as the_file:
		the_file.write("DROP TABLE IF EXISTS UDF_TEST;\n")
		the_file.write("CREATE TABLE UDF_TEST(a double precision);\n")

		for i in range(0, 625000):
			cmd = 'insert into UDF_TEST values(%s);\n' % (str(i*1.0))
			the_file.write(cmd)

		the_file.write("CREATE OR REPLACE FUNCTION increment(i double precision) RETURNS double precision AS $$ BEGIN RETURN i + 1; END; $$ LANGUAGE plpgsql;\n")
		the_file.write("select count(*) from UDF_TEST;\n");

