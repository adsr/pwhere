--TEST--
pxtrace test basic
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
%w%f%w 0 <internal>:-1   <main>
BEGIN
%w%f%w 1   %a:%d%w f1
f1
%w%f%w 2     %a:%d%w C::m1
m1
%w%f%w 3       %a:%d%w C::m2
m2
%w%f%w 4         %a:%d%w C::m3
m3
END
