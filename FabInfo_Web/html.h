
// HTML page to be served by the FabInfo webserver
// Conveniently, the Arduino C++ compiler supports raw-strings

static const char html[] = R"(
	<!DOCTYPE html>
	<html>
	  <head>
		<style> 
		  input.wide[type=text]{ 
			-webkit-appearance: none; 
			-moz-appearance: none; 
			display: block; 
			margin: 0; 
			width: 100%; 
			height: 40px; 
			line-height: 40px; 
			font-size: 17px; 
			border: 1px solid #bbb; }
		</style>
	  </head>
	  <body>
		<form action="/" method="GET">
		  Text der FabInfo-Anzeige:<br>
		  <input type="text" name="text" class="wide" value="%TEXT%">
		  <input type="text" name="speed" value="%SPEED%">  Geschwindigkeit (Pixel/Sek)<br>
		  <input type="text" name="repeat" value="%REPEAT%">  Wiederholungen (0: Unendlich)<br>
		  <input type="submit" value="Text Anzeigen!">
		</form>
	  </body>
	</html> 
)";
