<?PHP

// Configuration
define("API_ENDPOINT_AND_KEY","{api_endpoint_and_api_key}");

// Reterave the current departures
$plat1 = json_decode( file_get_contents(API_ENDPOINT_AND_KEY . "/1") );
$plat2 = json_decode( file_get_contents(API_ENDPOINT_AND_KEY . "/2") );

// Build a single list and sort by time
$plats = array();
foreach ( $plat1 as $key => $train ) {
	$plats[str_pad($train->dueIn,3,"0",STR_PAD_LEFT) . ".1."  . $key] = $train;
}
foreach ( $plat2 as $key => $train ) {
	$plats[str_pad($train->dueIn,3,"0",STR_PAD_LEFT) . ".2."  . $key] = $train;
}
ksort( $plats );

// Tidy up the text to make it more readable
$deplist = array();
foreach ( $plats as $key => $train ) {
	$deplist += array( $train->dueIn => ( ( $train->dueIn>0 ? $train->dueIn . "m" : "Now" ) . " - " . $train->destination ) );
}

// Output the marker for the start of the line
echo "###";

// Return the infromatrion as a readable single line
$count = 3;
foreach ( $deplist as $dueIn => $line ) {
	echo $line . "  ";
	if (--$count<=0) { break; }
}

?>