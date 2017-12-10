import os
import subprocess
import shlex
import time
from timeit import default_timer as timer

class Test:
	def __init__(self, isPeloton, setup_script, run_script, output):
		self.isPeloton = isPeloton
		self.setup_script = setup_script
		self.run_script = run_script
		self.output = output

	def cleanup(self):
		cmd = 'echo "drop table if exists UDF_TEST;" | psql postgres > /dev/null 2>&1'
		exit_code = os.system(cmd)

	# def start_postgres_server(self):
	# 	#path_to_build = "/Users/prashasthiprabhakar/Documents/2017/LLVM_UDF_CAPSONE/peloton/build"
	# 	#os.chdir(path_to_build)

	# 	cmd = "pg_ctl -D /usr/local/var/postgres -l /usr/local/var/postgres/server.log start > /dev/null &"
	# 	args = shlex.split(cmd)
	# 	# Start the server
	# 	self.proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
	# 	#print "Process ID: ", self.proc.pids
	def setup_udf_test(self):
		cmd = 'echo "\i %s" | psql postgres > %s 2>&1' % (self.setup_script, self.output)
		exit_code = os.system(cmd)
		return exit_code

	def run_udf(self):
		cmd = 'echo "\i %s" | psql postgres > %s 2>&1' % (self.run_script, self.output)
		exit_code = os.system(cmd)
		return exit_code

	def run_analyze(self, script_explain):
		cmd = 'echo "\i %s" | psql postgres > %s 2>&1' % (script_explain, "/tmp/explain_analyze.txt")
		exit_code = os.system(cmd)
		return exit_code

output = "/tmp/psql_out-1"
run_script = "../dml/udf_invocation.sql"
l_postgres = ["postgres_script1000.sql", "postgres_script5000.sql","postgres_script25000.sql", "postgres_script125000.sql"]
#l_postgres = ["postgres_script1000.sql", "postgres_script5000.sql", "postgres_script25000.sql", "postgres_script125000.sql", "postgres_script625000.sql"]

peloton_result = "postgres_result.txt"
pelotonfile = open(peloton_result,"w") 

pelotonfile.write("Postgres results\n\n");
for i in range(0, len(l_postgres)):
	z = l_postgres[i].split(".")
	pelotonfile.write("Number of Tuples : %s\n" % (z[0][15:]));
	pelotonfile.write("---------\n")
	test_script = "../dml/" + l_postgres[i]
	p_test = Test(False, test_script, run_script, output)
	print("Sleeping for 5 seconds...")
	# sleep for 5 seconds
	time.sleep(5)
	exit_code = p_test.setup_udf_test()
	for j in range(0, 5):
		pelotonfile.write("Try %d:\n" % (j))
		start = timer()
		p_test.run_udf()
		end = timer()
		print "Time taken UDF: ", end-start
		pelotonfile.write("%f seconds\n" % (end-start))
	pelotonfile.write("Explain analyze values\n")
	#for j in range(0, 5):
	#	pelotonfile.write("Try %d:\n" % (j))
	#	p_test.run_analyze()
	p_test.cleanup()
	pelotonfile.write("\n\n");

pelotonfile.close()








