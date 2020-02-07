# gtk方式

* -display gtk
* 不支持 -gtk 快捷参数
* 不是所有QEMU版本都支持gtk方式，具体哪个版本开始支持不清楚

```
$ qemu-system-i386 -display gtk ./tinycorelinux.img
```

![20200207_122504_41](image/20200207_122504_41.png)

可以看到用 GTK 绘制的窗口有两个菜单选项：虚拟机(M)、视图(V)。

![20200207_122714_89](image/20200207_122714_89.png)
