--TEST--
pxtrace test max_depth
--INI--
pxtrace.auto_enable=1
pxtrace.output_path=@sapi
pxtrace.max_depth=2
--EXTENSIONS--
pxtrace
--FILE--
<?php
function f1() { echo __FUNCTION__ . PHP_EOL; f2(); }
function f2() { echo __FUNCTION__ . PHP_EOL; f3(); }
function f3() { echo __FUNCTION__ . PHP_EOL; f4(); }
function f4() { echo __FUNCTION__ . PHP_EOL; }
echo "BEGIN\n";
f1();
echo "END\n";
?>
--EXPECTF--
%w%f%w 0 <internal>:-1   <main>
BEGIN
%w%f%w 1   %a:%d%w f1
f1
%w%f%w 2     %a:%d%w f2
f2
f3
f4
END
