GSModloader
==================================
GSModloader是一个用于加载多个Mod的GriefSyndrome Mod，并提供了部分API用于处理多Mod间的冲突。<br>
这个工程由 Acaly 和助手 夏幻 共同完成。

构建时请先将NatsuLib设为使用多字节字符集把解决方案编译一遍，然后设为Unicode字符集单独编译工程RemoteCommandSender。然后将生成的GSLoader.exe与NML.dll以及Script文件夹下的所有文件移至你的游戏目录下（若你希望使用快速输入命令的Mod，则你需要把生成的RemoteCommandSender.exe放在config文件夹下），执行GSLoader.exe即可启用GSModloader，注意当gs03.dat存在时GSModloader不会启用。