--TEST--
pxtrace test trace_statements
--INI--
pxtrace.auto_enable=1
pxtrace.output_path=@stdout
pxtrace.trace_statements=1
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
%w%f%w 0 <internal>:-1   <main>
    %w 1   %a:%d%w echo "BEGIN\n";
f1
    %w 1   %a:%d%w f1();
%w%f%w 1   %a:%d%w f1
    %w 2     %a:%d%w echo __FUNCTION__ . PHP_EOL;
m1
    %w 2     %a:%d%w $c = new C();
    %w 2     %a:%d%w $c->m1();
%w%f%w 2     %a:%d%w C::m1
    %w 3       %a:%d%w echo __FUNCTION__ . PHP_EOL;
m2
    %w 3       %a:%d%w $this->m2();
%w%f%w 3       %a:%d%w C::m2
    %w 4         %a:%d%w echo __FUNCTION__ . PHP_EOL;
m3
    %w 4         %a:%d%w self::m3();
%w%f%w 4         %a:%d%w C::m3
    %w 5           %a:%d%w echo __FUNCTION__ . PHP_EOL;
END
    %w 1   %a:%d%w echo "END\n";
