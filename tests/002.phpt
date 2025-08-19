--TEST--
pxtrace test pxtrace_set_enabled
--INI--
pxtrace.output_path=@stdout
pxtrace.auto_enable=0
--EXTENSIONS--
pxtrace
--FILE--
<?php
class C {
    public function m1() {
        pxtrace_set_enabled(true);
        echo __FUNCTION__ . PHP_EOL;
        $this->m2();
    }
    private function m2() {
        echo __FUNCTION__ . PHP_EOL;
        pxtrace_set_enabled(false);
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
f1
m1
m2
%w%f%w3       %s:%d C::m2
m3
END
