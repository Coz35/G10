<?php
include_once 'includes/db_connect.php';
include_once 'includes/functions.php';
 
sec_session_start();
?>
<!doctype html> 
<html>
<head>
<meta charset="UTF-8">

<title>G10 IOT Data Mule</title>

<meta name="viewport" content="width=device-width, initial-scale=1">
<link href="styles/boilerplate.css" rel="stylesheet" type="text/css">
<link href="styles/index.css" rel="stylesheet" type="text/css">
<!-- 
To learn more about the conditional comments around the html tags at the top of the file:
paulirish.com/2008/conditional-stylesheets-vs-css-hacks-answer-neither/

Do the following if you're using your customized build of modernizr (http://www.modernizr.com/):
* insert the link to your js here
* remove the link below to the html5shiv
* add the "no-js" class to the html tags at the top
* you can also remove the link to respond.min.js if you included the MQ Polyfill in your modernizr build 
-->
<!--[if lt IE 9]>
<script src="//html5shiv.googlecode.com/svn/trunk/html5.js"></script>
<![endif]-->
<!--Load the AJAX API-->
    <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
    <script type="text/javascript" src="//ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js"></script>
    <script type="text/javascript">

      // Load the Visualization API and the corechart package.
      google.charts.load('current', {'packages':['corechart']});

      // Set a callback to run when the Google Visualization API is loaded.
      google.charts.setOnLoadCallback(drawChart);

      // Callback that creates and populates a data table,
      // instantiates the pie chart, passes in the data and
      // draws it.
	  var data;
	  var options;
      function drawChart() {

        var jsonData = $.ajax({
          url: "getTempData.php",
          dataType: "json",
          async: false
          }).responseText;
		  
          
      // Create our data table out of JSON data loaded from server.
		  
      var data = new google.visualization.DataTable(jsonData);


        // Set chart options
        options = {'title':'Temperature Data',
                       
                       'height':600
					   };

        // Instantiate and draw our chart, passing in some options.
        var chart = new google.visualization.LineChart(document.getElementById('chart_div'));
        chart.draw(data, options);
      }
	  
	  
  function resizeChart () {
    chart.draw(data, options);  
	}
	if (document.addEventListener) {
		window.addEventListener('resize', drawChart);
	}
	else if (document.attachEvent) {
		window.attachEvent('onresize', drawChart);
	}
	else {
		window.resize = resizeChart;
	}
 </script>
    
<script src="styles/respond.min.js"></script>
</head>
 

<body>
<?php if (login_check($mysqli) == true) : ?>
            
            
	
	<div class="gridContainer clearfix">
  
  <div align="center" style="padding-top: 20px" id="Image" >
	<a href="home.php"><img src="/images/logo.png"></a> 
	  
  </div>
	<div align="center" >
		<H2>Welcome <?php echo htmlentities($_SESSION['username']); ?>!  </h2>
	
	</div>
  <div align="center" id="Body">
	  
    <h1>IOT Data Mule Drone Project </H1>
  </div>
  <div align="center" id="chart_div"></div>
  
</div>
        <?php else : ?>
            <p>
                <span class="error">You are not authorized to access this page.</span> Please <a href="index.php">login</a>.
            </p>
        <?php endif; ?>




</body>
</html>
