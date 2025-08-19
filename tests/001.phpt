--TEST--
pxtrace test
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
?>
--EXPECTF--
BEGIN
%w%f%w0 <internal>:-1 <main>
f1
%w%f%w1   %s:%d f1
m1
%w%f%w2     %s:%d C::m1
m2
%w%f%w3       %s:%d C::m2
m3
%w%f%w4         %s:%d C::m3
END
