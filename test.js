var b = require('./build/Debug/shim');
console.log(b);
console.log("test_func ret", b.test_func(42, 42));
b.test_foo("hello world");
