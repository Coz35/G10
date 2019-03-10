<?php 
	include "../inc/dbinfo.inc"; 

	/* Connect to MySQL and select the database. */
	$connection = mysqli_connect(DB_SERVER, DB_USERNAME, DB_PASSWORD);
 
	if (mysqli_connect_errno()) echo "Failed to connect to MySQL: " . mysqli_connect_error();

	$database = mysqli_select_db($connection, DB_DATABASE);

	/* Ensure that the  table exists. */
	VerifyTable($connection, DB_DATABASE); 

	$columns[] = array(id=>"", label=>"datasetdate",pattern=>"",type=>"string");
	$columns[] = array(id=>"", label=>"temperature",pattern=>"",type=>"number");

	//$rows[] = array(c=>array(array(v=>"Mushrooms", f=>null),array(v=>3,f=>null)));
	
	


	$result = mysqli_query($connection, 'SELECT round(avg(temperature),2), date_format(datasetdate, "%i") datasetdate FROM temperatures group by date_format(datasetdate, "%i")  order by datasetdate'); 

	while($query_data = mysqli_fetch_row($result)) {

	  $rows[] = array(c=>array(array(v=>$query_data[1], f=>null),array(v=>$query_data[0],f=>null)));


	}
	$arr = array(cols=>$columns, rows=>$rows);
	print  json_encode($arr);
 
	mysqli_free_result($result);
	mysqli_close($connection);










	/* Check whether the table exists and, if not, create it. */
	function VerifyTable($connection, $dbName) 
	{
	  if(!TableExists("temperatures", $connection, $dbName)) 
	  { 
		 echo("<p>Error table does not exist.</p>");
	  }
	}

	/* Check for the existence of a table. */
	function TableExists($tableName, $connection, $dbName) 
	{
	  $t = mysqli_real_escape_string($connection, $tableName);
	  $d = mysqli_real_escape_string($connection, $dbName);

	  $checktable = mysqli_query($connection, 
		  "SELECT TABLE_NAME FROM information_schema.TABLES WHERE TABLE_NAME = '$t' AND TABLE_SCHEMA = '$d'");

	  if(mysqli_num_rows($checktable) > 0) return true;

	  return false;
	}
?>
