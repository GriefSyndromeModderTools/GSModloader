//!	请勿随意修改本文件

// NOTE: raw指是否按数据的机内表示输出（string除外），不影响readn及writen，按raw输出的数据可以直接被FileReader读取
// 不支持读取ascii以外的字符
class FileClass
{
	fstream = null;
	fname = null;
	mode = null;
	raw = false;
	constructor(filename, mode, raw = false) {
		if (!(this.fstream = ::file(filename, mode)))
		{
			throw "open file failed";
		}
		else
		{
			this.fname = filename;
			this.mode = mode;
			this.raw = raw;
		}
	}

	function setraw(value = true) {
		raw = value;
	}

	function readn(type) {
	 	return this.fstream.readn(type);
	}

	function readblob(size) {
		return this.fstream.readblob(size);
	}

	function len() {
		return this.fstream.len();
	}

	function eof() {
		return this.fstream.eos();
	}

	function flush() {
		return this.fstream.flush();
	}

	function seek(offset, origin = 'b') {
		return this.fstream.seek(offset, origin);
	}

	function tell() {
		return this.fstream.tell();
	}

	function writen(n, type) {
		return this.fstream.writen(n, type);
	}

	function writeblob(blob) {
		return this.fstream.writeblob(blob);
	}

	function readint() {
		if (raw)
		{
			return this.fstream.readn('i');
		}
		else
		{
			return this.readstring().tointeger();
		}
	}

	function readfloat() {
		if (raw)
		{
			return this.fstream.readn('f');
		}
		else
		{
			return this.readstring().tofloat();
		}
	}

	function readstring() {
		if (raw)
		{
			local len = this.fstream.readn('i');
			local tmpstr = "";

			for (; len > 0; --len) {
				tmpstr += this.fstream.readn('c').tochar();
			}

			return tmpstr;
		}
		else
		{
			local tmpstr = "";
			local tmpchar;
			local ex = ::regexp(@"\s");

			while (!this.fstream.eos()) {
				tmpchar = this.fstream.readn('c').tochar();
				if (ex.match(tmpchar))
				{
					break;
				}

				tmpstr += tmpchar;
			}

			return tmpstr;
		}
	}

	function readbool() {
		if (raw)
		{
			return this.fstream.readn('b');
		}
		else
		{
			local tmpvar = this.readstring();
			if (tmpvar == "true")
			{
				return true;
			}
			else if (tmpvar == "false")
			{
				return false;
			}
			else
			{
				throw "type error";
			}
		}
	}

	function writeint(value) {
		if (type(value) != "integer")
		{
			throw "invalid argument";
		}

		if (raw)
		{
			this.fstream.writen(value, 'i');
		}
		else
		{
			this.writestring(value.tostring());
		}
	}

	function writefloat(value) {
		if (type(value) != "float")
		{
			throw "invalid argument";
		}

		if (raw)
		{
			this.fstream.writen(value, 'f');
		}
		else
		{
			this.writestring(value.tostring());
		}
	}

	function writestring(value) {
		if (type(value) != "string")
		{
			throw "invalid argument";
		}

		if (raw)
		{
			this.fstream.writen(value.len(), 'i');
		}

		foreach (x in value) {
			this.fstream.writen(x, 'c');
		}
	}

	function writebool(value) {
		if (type(value) != "bool")
		{
			throw "invalid argument";
		}

		if (raw)
		{
			this.fstream.writen(value, 'b');
		}
		else
		{
			this.writestring(value.tostring());
		}
	}

	function writedata(value) {
		local vtype = type(value);

		switch (vtype)
		{
		case "integer":
			this.writeint(value);
			break;
		case "float":
			this.writefloat(value);
			break;
		case "bool":
			this.writebool(value);
			break;
		case "string":
			this.writestring(value);
			break;
		default:
			this.writestring(type(value) != "instance" ? value.tostring() : typeof(value));
		}

		return this;
	}

	function _sub(value) {
		this.writedata(value);
		return this;
	}
}