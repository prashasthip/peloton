//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// UDFPerformanceTest.java
//
// Identification: script/testing/jdbc/src/PelotonTest.java
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.sql.*;
import java.time.*;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;
/**
 *
 * @author prashasthiprabhakar
 */
public class UDFTestPerformance {
    
    // For Postgres
    private final String postgres_url = "jdbc:postgresql://localhost/postgres";
    private final String postgres_user = "prashasthiprabhakar";
    private final String postgres_password = "";
//    
    //For Peloton
    private final String peloton_url = "jdbc:postgresql://localhost:15721/";
    private final String peloton_user = "postgres";
    private final String peloton_password = "postgres";
    
    private final String DDL_PELOTON = "CREATE TABLE UDF_TEST (a double);";
    private final String DDL_POSTGRES = "CREATE TABLE UDF_TEST (a double precision);";
    private final String TEMPLATE_FOR_BATCH_INSERT = "INSERT INTO UDF_TEST VALUES (?);";
    
    private final String DROP_TABLE = "DROP TABLE IF EXISTS UDF_TEST;";
    private final Connection conn;
    
    private final PrintWriter print_writer;
    
    boolean isPeloton;
    
    public UDFTestPerformance(boolean isPeloton) throws SQLException, IOException {
        this.isPeloton = isPeloton;
        if(isPeloton) {
            print_writer =  new PrintWriter("Peloton_release_results.txt");
        } else {
            print_writer =  new PrintWriter("Postgres_release_results.txt");
        }
        
        conn = this.makeConnection();
    }
    
    public void Init() throws SQLException {
        conn.setAutoCommit(true);
        Statement stmt = conn.createStatement();
        if(isPeloton) {
           stmt.execute(DDL_PELOTON);
        } else {
           stmt.execute(DDL_POSTGRES); 
        }
        
    }
    
    private Connection makeConnection() throws SQLException{
        if(isPeloton) {
            Connection conn1 = DriverManager.getConnection(peloton_url, peloton_user, peloton_password);
            System.out.println("Connected to the Peloton server successfully.");
            
            return conn1;
        } else {
            Connection conn1 = DriverManager.getConnection(postgres_url, postgres_user, postgres_password);
            System.out.println("Connected to the PostgreSQL server successfully.");
            
            return conn1;
        }
       
    }
    
    private void displayInfo(ResultSet rs) throws SQLException{
        while (rs.next()) {
            System.out.println(rs.getDouble(1));
        }
    }
    
    public void cleanup() throws SQLException {
        conn.setAutoCommit(true);
        Statement stmt = conn.createStatement();
        stmt.execute(DROP_TABLE);
    }
    
    public void batchInsert(int val) throws Exception {
       PreparedStatement stmt = conn.prepareStatement(TEMPLATE_FOR_BATCH_INSERT);
       System.out.println("Starting Batch Insert");
        conn.setAutoCommit(false);
        double x;
        for(int i=1; i <= val ;i++){
          x = i * 1.0;
          stmt.setDouble(1,x);
          stmt.addBatch();
        }

        int[] res;
        try{
           res = stmt.executeBatch();
        }catch(SQLException e){
          throw e.getNextException();
        }
        /*for(int i=0; i < res.length; i++){
          //System.out.println(res[i]);
          if (res[i] < 0) {
            throw new SQLException("Query "+ (i+1) +" returned " + res[i]);
          }
        }*/
        System.out.println("Batch Insert Done");
        conn.commit(); 

    }
    
    public void createUdf() throws SQLException{
        String SQL = "";
        if(isPeloton) {
            SQL = "CREATE OR REPLACE FUNCTION increment(i double) RETURNS double AS $$ BEGIN RETURN i + 1; END; $$ LANGUAGE plpgsql;";
            System.out.println("Creating UDF in Peloton");
            //print_writer.println("Creating UDF in Peloton");
        } else {
           SQL = "CREATE OR REPLACE FUNCTION increment(i double precision) RETURNS double precision AS $$ BEGIN RETURN i + 1; END; $$ LANGUAGE plpgsql;"; 
           System.out.println("Creating UDF in Postgres");
           //print_writer.println("Creating UDF in Postgres");
        }
        
        conn.setAutoCommit(true);
        Statement stmt = conn.createStatement();
        stmt.execute(SQL); 
        System.out.println("Created UDF");
    }
    
    public void executeUDF() throws SQLException {
        //System.out.println("Executing the UDF...");
        //print_writer.println("Executing UDF");
        String SQL = "select increment(a) from UDF_TEST;"; 
        conn.setAutoCommit(true);
        Statement stmt = conn.createStatement();
        long startTime = System.currentTimeMillis();
        stmt.execute(SQL); 
        long stopTime = System.currentTimeMillis();
        
        long elapsedTime = stopTime - startTime;
        //System.out.println(elapsedTime + " Milliseconds");
        print_writer.println(elapsedTime + " Milliseconds");
        
        ResultSet rs = stmt.getResultSet();
        
        //displayInfo(rs);
    }

    public void simpleSelect() throws SQLException {
        String SQL = "select * from UDF_TEST;";
        conn.setAutoCommit(true);
        Statement stmt = conn.createStatement();
    }

    /**
     * @param args the command line arguments
     * @throws java.sql.SQLException
     */
    public static void main(String[] args) throws SQLException, Exception {
        ArrayList<Integer> num_tuples = new ArrayList<Integer>();
        bool isPel = true;
        num_tuples.add(1);
        num_tuples.add(1000);
        // num_tuples.add(5000);
        // num_tuples.add(25000);
        // num_tuples.add(125000);
        // num_tuples.add(625000);
        //num_tuples.add(3125000);
        //num_tuples.add(10000000);
        UDFTestPerformance app = new UDFTestPerformance(isPel);
        
        for(Integer j: num_tuples) {
            app.Init();
            app.batchInsert(j);
            app.print_writer.println("Number of Tuples: " + j);
            System.out.println("Number of Tuples: " + j);
            app.createUdf();
            app.simpleSelect();
            app.print_writer.println("End to End execution ");
            System.out.println("End to End execution ");
           for(int i=0; i< 3; i++) {
                 app.print_writer.println("Try " + (i+1));
                 System.out.println("Try " + (i+1));
                 TimeUnit.SECONDS.sleep(2);
                 app.executeUDF();
            }
            app.print_writer.println("Just execution times ");
            for(int i=0; i< 3; i++) {
                 app.print_writer.println("Try " + (i+1));
                 TimeUnit.SECONDS.sleep(2);
                 app.executeUDF();
            }
            TimeUnit.SECONDS.sleep(30);
            app.cleanup();
            app.print_writer.println("--------------------");
        }
       app.print_writer.close();
    }
    
}