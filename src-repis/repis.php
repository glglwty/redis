<?php 
$_PROG = new t_program("repis");

$_PROG->namespaces["cpp"] = "dsn.apps";

$tmp = new t_program("dsn");
$_PROG->includes[$tmp->name] = $tmp;
$tmp->namespaces["cpp"] = "dsn";

$tmp = new t_service($_PROG, "repis");
$tmp2 = $tmp->add_function("dsn.blob", "read");
$tmp2->add_param("request", "dsn.blob");
$tmp2 = $tmp->add_function("dsn.blob", "write");
$tmp2->add_param("request", "dsn.blob");

?>
<?php
$_PROG->add_annotations(Array(
	"service.rrdb" => Array(
		"stateful" => "1",
	),
	"function.repis.read" => Array(
		"write" => "",
	),
	"function.repis.write" => Array(
		"write" => "1",
	),
));
?>
