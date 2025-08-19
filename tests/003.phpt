--TEST--
pxtrace test pxtrace.output_path
--INI--
pxtrace.output_path=@stdout
pxtrace.auto_enable=1
--EXTENSIONS--
pxtrace
--FILE--
<?php
class C {
    public function m1() {
        echo __FUNCTION__ . PHP_EOL;
        ini_set('pxtrace.output_path', __FILE__ . '.tmp');
        $this->m2();
    }
    private function m2() {
        echo __FUNCTION__ . PHP_EOL;
        self::m3();
    }
    private static function m3() {
        echo __FUNCTION__ . PHP_EOL;
    }
}
function f1() {
    echo __FUNCTION__ . PHP_EOL;
    $c = new C();
    $c->m1();
}
echo "BEGIN\n";
f1();
echo "END\n";

echo "TMP\n";
pxtrace_set_enabled(false); // needed to flush output to tmp file
echo file_get_contents(__FILE__ . '.tmp');
?>
--CLEAN--
<?php @unlink(__FILE__ . '.tmp'); ?>
--EXPECTF--
BEGIN
%w%f%w0 <internal>:-1 <main>
f1
%w%f%w1   %s:%d f1
m1
%w%f%w2     %s:%d C::m1
m2
m3
END
TMP
%w%f%w3       %s:%d C::m2
%w%f%w4         %s:%d C::m3
