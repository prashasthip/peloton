import os
import subprocess
import shlex
import time
from timeit import default_timer as timer

class Test:
	def __init__(self, isPeloton, port, setup_script, run_script):
		self.isPeloton = isPeloton
		self.port = port
		self.setup_script = setup_script
		self.run_script = run_script

	def cleanup(self):
		cmd = 'echo "drop table if exists UDF_TEST;" | psql sslmode=disable -U postgres -h localhost -p %s > /dev/null 2>&1' % (self.port)
		os.system(cmd)

	# def start_peloton_server(self):
	# 	#path_to_build = "/Users/prashasthiprabhakar/Documents/2017/LLVM_UDF_CAPSONE/peloton/build"
	# 	#os.chdir(path_to_build)

	# 	cmd = "~/users/prashasp/peloton/build/bin/peloton -port %s > /dev/null &" % (self.port)
	# 	args = shlex.split(cmd)
	# 	# Start the server
	# 	self.proc = subprocess.Popen(args)

	# 	#print "Process ID: ", self.proc.pid

	def setup_udf_test(self):
		cmd = 'echo "\i %s" | psql sslmode=disable -U postgres -h localhost -p %s > %s 2>&1' % (self.setup_script, self.port, "/tmp/setup.txt")
		exit_code = os.system(cmd)
		return exit_code

	def run_udf(self):
		cmd = 'echo "\i %s" | psql sslmode=disable -U postgres -h localhost -p %s > %s 2>&1' % (self.run_script, self.port, "/tmp/invoke.txt")
		exit_code = os.system(cmd)

l_peloton = ["peloton_script1000.sql", "peloton_script5000.sql", "peloton_script25000.sql", "peloton_script125000.sql"]
#l_peloton = ["peloton_script1000.sql", "peloton_script5000.sql", "peloton_script25000.sql", "peloton_script125000.sql", "peloton_script625000.sql"]
#l_postgres = ["postgres_script1000.sql", "postgres_script5000.sql", "postgres_script25000.sql", "postgres_script125000.sql", "postgres_script625000.sql"]

peloton_result = "peloton_result.txt"
pelotonfile = open(peloton_result,"w") 

port = "15721"
#run_script = "~/users/prashasp/peloton/script/testing/dml/udf_invocation.sql"
run_script = "../dml/udf_invocation.sql"

pelotonfile.write("Peloton results\n\n");
for i in range(0, len(l_peloton)):
	pelotonfile.write("Number of Tuples : %s\n" % (l_peloton[i][14:18]));
	pelotonfile.write("---------\n")
	test_script = "../dml/" + l_peloton[i]
	#test_script = "~/users/prashasp/peloton/script/testing/dml/" + l_peloton[i]
	p_test = Test(True, port, test_script, run_script)
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
	p_test.cleanup()
	pelotonfile.write("\n\n");

pelotonfile.close()


