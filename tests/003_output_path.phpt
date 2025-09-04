--TEST--
pxtrace test output_path
--INI--
pxtrace.auto_enable=1
pxtrace.output_path=@sapi
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
pxtrace_set_enabled(false); // flush output to tmp file
echo file_get_contents(__FILE__ . '.tmp');
?>
--CLEAN--
<?php @unlink(__FILE__ . '.tmp'); ?>
--EXPECTF--
%w%f%w 0 <internal>:-1   <main>
BEGIN
%w%f%w 1   %a:%d%w f1
f1
%w%f%w 2     %a:%d%w C::m1
m1
m2
m3
END
TMP
%w%f%w 3       %a:%d%w C::m2
%w%f%w 4         %a:%d%w C::m3
