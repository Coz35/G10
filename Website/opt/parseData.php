#!/usr/bin/php
<?php


	include "../www/inc/dbinfo.inc";


	/* Connect to MySQL and select the database. */
	$connection = mysqli_connect(DB_SERVER, DB_USERNAME, DB_PASSWORD);

	if (mysqli_connect_errno()) echo "Failed to connect to MySQL: " . mysqli_connect_error();

	$database = mysqli_select_db($connection, DB_DATABASE);


	//Database and table info
	$table = 'DatamuleDB.temperatures';


	//check for text files
	$directory = '../../var/NodeData/*.txt';
	$files = glob($directory);
	//print_r($files);
	
	
	
	foreach($files as $file)
	{
		$txt_file    = file_get_contents($file);
		$rows        = explode("\n", $txt_file);
		array_shift($rows);
		$recompiled = "";
		foreach($rows as $row => $data)
		{
			//get row data
			$row_data = explode("%NOTI,0029,",  $data);
			foreach($row_data as $line)
			{

				if(!(strpos($line, 'CMD>') !== false))
				{
					$line = substr($line, 0, strpos($line, "%"));
					$recompiled .= hex2str($line);

				}
			}
		}
		$hexsplit = explode(",", $recompiled);
		
		//print_r($hexsplit);
		echo $recompiled . "\n\n";
	}


function hex2str($hex) {
    $str = '';
    for($i=0;$i<strlen($hex);$i+=2) $str .= chr(hexdec(substr($hex,$i,2)));
    return $str;
}
?>
