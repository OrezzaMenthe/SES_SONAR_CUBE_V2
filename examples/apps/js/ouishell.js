var MD5 = function(s){function L(k,d){return(k<<d)|(k>>>(32-d))}function K(G,k){var I,d,F,H,x;F=(G&2147483648);H=(k&2147483648);I=(G&1073741824);d=(k&1073741824);x=(G&1073741823)+(k&1073741823);if(I&d){return(x^2147483648^F^H)}if(I|d){if(x&1073741824){return(x^3221225472^F^H)}else{return(x^1073741824^F^H)}}else{return(x^F^H)}}function r(d,F,k){return(d&F)|((~d)&k)}function q(d,F,k){return(d&k)|(F&(~k))}function p(d,F,k){return(d^F^k)}function n(d,F,k){return(F^(d|(~k)))}function u(G,F,aa,Z,k,H,I){G=K(G,K(K(r(F,aa,Z),k),I));return K(L(G,H),F)}function f(G,F,aa,Z,k,H,I){G=K(G,K(K(q(F,aa,Z),k),I));return K(L(G,H),F)}function D(G,F,aa,Z,k,H,I){G=K(G,K(K(p(F,aa,Z),k),I));return K(L(G,H),F)}function t(G,F,aa,Z,k,H,I){G=K(G,K(K(n(F,aa,Z),k),I));return K(L(G,H),F)}function e(G){var Z;var F=G.length;var x=F+8;var k=(x-(x%64))/64;var I=(k+1)*16;var aa=Array(I-1);var d=0;var H=0;while(H<F){Z=(H-(H%4))/4;d=(H%4)*8;aa[Z]=(aa[Z]| (G.charCodeAt(H)<<d));H++}Z=(H-(H%4))/4;d=(H%4)*8;aa[Z]=aa[Z]|(128<<d);aa[I-2]=F<<3;aa[I-1]=F>>>29;return aa}function B(x){var k="",F="",G,d;for(d=0;d<=3;d++){G=(x>>>(d*8))&255;F="0"+G.toString(16);k=k+F.substr(F.length-2,2)}return k}function J(k){k=k.replace(/rn/g,"n");var d="";for(var F=0;F<k.length;F++){var x=k.charCodeAt(F);if(x<128){d+=String.fromCharCode(x)}else{if((x>127)&&(x<2048)){d+=String.fromCharCode((x>>6)|192);d+=String.fromCharCode((x&63)|128)}else{d+=String.fromCharCode((x>>12)|224);d+=String.fromCharCode(((x>>6)&63)|128);d+=String.fromCharCode((x&63)|128)}}}return d}var C=Array();var P,h,E,v,g,Y,X,W,V;var S=7,Q=12,N=17,M=22;var A=5,z=9,y=14,w=20;var o=4,m=11,l=16,j=23;var U=6,T=10,R=15,O=21;s=J(s);C=e(s);Y=1732584193;X=4023233417;W=2562383102;V=271733878;for(P=0;P<C.length;P+=16){h=Y;E=X;v=W;g=V;Y=u(Y,X,W,V,C[P+0],S,3614090360);V=u(V,Y,X,W,C[P+1],Q,3905402710);W=u(W,V,Y,X,C[P+2],N,606105819);X=u(X,W,V,Y,C[P+3],M,3250441966);Y=u(Y,X,W,V,C[P+4],S,4118548399);V=u(V,Y,X,W,C[P+5],Q,1200080426);W=u(W,V,Y,X,C[P+6],N,2821735955);X=u(X,W,V,Y,C[P+7],M,4249261313);Y=u(Y,X,W,V,C[P+8],S,1770035416);V=u(V,Y,X,W,C[P+9],Q,2336552879);W=u(W,V,Y,X,C[P+10],N,4294925233);X=u(X,W,V,Y,C[P+11],M,2304563134);Y=u(Y,X,W,V,C[P+12],S,1804603682);V=u(V,Y,X,W,C[P+13],Q,4254626195);W=u(W,V,Y,X,C[P+14],N,2792965006);X=u(X,W,V,Y,C[P+15],M,1236535329);Y=f(Y,X,W,V,C[P+1],A,4129170786);V=f(V,Y,X,W,C[P+6],z,3225465664);W=f(W,V,Y,X,C[P+11],y,643717713);X=f(X,W,V,Y,C[P+0],w,3921069994);Y=f(Y,X,W,V,C[P+5],A,3593408605);V=f(V,Y,X,W,C[P+10],z,38016083);W=f(W,V,Y,X,C[P+15],y,3634488961);X=f(X,W,V,Y,C[P+4],w,3889429448);Y=f(Y,X,W,V,C[P+9],A,568446438);V=f(V,Y,X,W,C[P+14],z,3275163606);W=f(W,V,Y,X,C[P+3],y,4107603335);X=f(X,W,V,Y,C[P+8],w,1163531501);Y=f(Y,X,W,V,C[P+13],A,2850285829);V=f(V,Y,X,W,C[P+2],z,4243563512);W=f(W,V,Y,X,C[P+7],y,1735328473);X=f(X,W,V,Y,C[P+12],w,2368359562);Y=D(Y,X,W,V,C[P+5],o,4294588738);V=D(V,Y,X,W,C[P+8],m,2272392833);W=D(W,V,Y,X,C[P+11],l,1839030562);X=D(X,W,V,Y,C[P+14],j,4259657740);Y=D(Y,X,W,V,C[P+1],o,2763975236);V=D(V,Y,X,W,C[P+4],m,1272893353);W=D(W,V,Y,X,C[P+7],l,4139469664);X=D(X,W,V,Y,C[P+10],j,3200236656);Y=D(Y,X,W,V,C[P+13],o,681279174);V=D(V,Y,X,W,C[P+0],m,3936430074);W=D(W,V,Y,X,C[P+3],l,3572445317);X=D(X,W,V,Y,C[P+6],j,76029189);Y=D(Y,X,W,V,C[P+9],o,3654602809);V=D(V,Y,X,W,C[P+12],m,3873151461);W=D(W,V,Y,X,C[P+15],l,530742520);X=D(X,W,V,Y,C[P+2],j,3299628645);Y=t(Y,X,W,V,C[P+0],U,4096336452);V=t(V,Y,X,W,C[P+7],T,1126891415);W=t(W,V,Y,X,C[P+14],R,2878612391);X=t(X,W,V,Y,C[P+5],O,4237533241);Y=t(Y,X,W,V,C[P+12],U,1700485571);V=t(V,Y,X,W,C[P+3],T,2399980690);W=t(W,V,Y,X,C[P+10],R,4293915773);X=t(X,W,V,Y,C[P+1],O,2240044497);Y=t(Y,X,W,V,C[P+8],U,1873313359);V=t(V,Y,X,W,C[P+15],T,4264355552);W=t(W,V,Y,X,C[P+6],R,2734768916);X=t(X,W,V,Y,C[P+13],O,1309151649);Y=t(Y,X,W,V,C[P+4],U,4149444226);V=t(V,Y,X,W,C[P+11],T,3174756917);W=t(W,V,Y,X,C[P+2],R,718787259);X=t(X,W,V,Y,C[P+9],O,3951481745);Y=K(Y,h);X=K(X,E);W=K(W,v);V=K(V,g)}var i=B(Y)+B(X)+B(W)+B(V);return i.toLowerCase()};

class User
{
	constructor(name, group, home)
	{
		this.name = name;
		this.group = group;
		this.home = home;
	}
};

class Authenticate
{
	constructor(challenge)
	{
		this.uploadXHR = new XMLHttpRequest();
		this.challenge = challenge;
		this.user = undefined;
		this.username = undefined;
		this.password = undefined;
		this.authorization = undefined;
		this.method = "GET";
		this.url = location.pathname.replace(/\\/g,'/').replace(/\/[^\/]*$/, '')
		this.islog = false;
		this.onauthorization = undefined;
		this.onauthenticate = undefined;
		this.onnotfound = undefined;
	}

	basic()
	{
		return "Basic "+window.btoa(this.username+":"+this.password);
	}
	digest()
	{
		var realm = this.challenge.search("realm=").split(" ")[0].split("=");
		var uri = this.url;
		var nonce = this.challenge.search("nonce=").split(" ")[0].split("=");
		var qop = this.challenge.search("qop=").split(" ")[0].split("=");
		var digest = "";
		var a1=this.username+":"+realm+":"+this.password;
		var a2=this.method+":"+uri;
		digest=atob(MD5(a1))+":"+nonce+":";
		if (qop === "auth")
			digest += this.nc+":"+this.cnonce+":";
		digest+=atob(MD5(a2));
		return digest;
	}

	build()
	{
		if (this.challenge.search("Digest") > -1)
			this.authorization = this.digest();
		else if (this.challenge.search("Basic") > -1)
			this.authorization = this.basic();
		this.result = "failed";
	}

	destroy()
	{
		document.cookie = "Authorization=;";
		this.authorization = undefined;
		this.result = "logout";
		this.user = undefined;
	}
	get()
	{
		const self = this;
		const xhr = self.uploadXHR;

		xhr.onreadystatechange = function()
		{
			if (xhr.readyState === XMLHttpRequest.DONE)
			{
				if (xhr.status === 200)
				{
					if (self.authorization && self.authorization.length > 0)
						document.cookie = "Authorization="+self.authorization+";"+document.cookie;
					var username = xhr.getResponseHeader("X-Remote-User");
					if (username != undefined)
					self.user = new User(username, xhr.getResponseHeader("X-Remote-Group"), xhr.getResponseHeader("X-Remote-Home"));
					self.islog = true;
					if (self.onauthorization != undefined)
						self.onauthorization.call(self, self.user);
				}
				else if (xhr.status === 403)
				{
					var challenge = xhr.getResponseHeader("WWW-Authenticate");
					self.authorization = undefined;
					if (challenge != undefined)
						self.challenge = challenge;
					self.islog = false;
					if (self.onauthenticate != undefined)
						self.onauthenticate.call(self, self.challenge, self.result);
				}
				else if (xhr.status === 404)
				{
					if (self.onnotfound != undefined)
						self.onnotfound.call(self);
				}
				else if (xhr.status > 0)
				{
					if (self.onerror != undefined)
					{
						self.onerror.call(self, xhr.status);
					}
				}
			}
			return true;
		}
		xhr.open(self.method, self.url, true);
		if (self.authorization != undefined)
		{
			xhr.withCredentials = true;
			xhr.setRequestHeader("Authorization", self.authorization);
		}
		xhr.setRequestHeader("X-Requested-With", "XMLHttpRequest");
		xhr.send();
	}
};
class Open
{
	constructor(root)
	{
		this.root = root;
		this.uploadXHR = new XMLHttpRequest();
		this.isready = false;
		this.onload = undefined;
		this.onauthenticate = undefined;
		this.onnotfound = undefined;
	}

	open(directory)
	{
		this.directory = this.root;
		this.directory += directory
		if (directory[directory.length - 1] != '/')
			this.directory += '/';
	}
	set(file)
	{
		this.file = file;
	}
	exec(authorization)
	{
		const self = this;
		const xhr = self.uploadXHR;

		xhr.onreadystatechange = function()
		{
			if (xhr.readyState === XMLHttpRequest.DONE)
			{
				if (xhr.status === 200)
				{
					self.isready = false;
					if (self.onload != undefined && xhr.response.type == "text/json")
					{
						//var data = String.fromCharCode.apply(null, xhr.response);
						var reader = new FileReader();
						reader.addEventListener("loadend", function (evt)
							{
								const array = new Uint8ClampedArray(evt.target.result);
								var result = new TextDecoder("utf-8").decode(array);
								var resultJson = JSON.parse(result);
								self.onload.call(self, resultJson);
							});
						reader.readAsArrayBuffer(xhr.response);
					}
					else if (self.onload != undefined)
					{
						xhr.response.name = self.file.name;
						xhr.response.newname = self.file.newname;
						xhr.response.oldname = self.file.oldname;
						self.onload.call(self, xhr.response);
					}
				}
				else if (xhr.status === 403)
				{
					if (self.onauthenticate != undefined)
						self.onauthenticate.call(self, xhr.getResponseHeader("WWW-Authenticate"), "logout");
				}
				else if (xhr.status === 404)
				{
					if (self.onnotfound != undefined)
					{
						var url = document.createElement('a');
						url .href = xhr.responseURL;
						self.onnotfound.call(self, url);
					}
				}
				else if (xhr.status > 0)
				{
					if (self.onerror != undefined)
					{
						self.onerror.call(self, xhr.status);
					}
				}
			}
			return true;
		}
		var target = self.directory;
		if (self.file.name == undefined)
			self.file.name = "";
		target += self.file.name; 
		
		xhr.open("GET", target);
		//xhr.responseType = "arraybuffer";
		xhr.responseType = "blob";
		xhr.withCredentials = true;
		xhr.setRequestHeader("X-Requested-With", "XMLHttpRequest");
		if (authorization)
			xhr.setRequestHeader("Authorization",authorization);
		xhr.send();
	}

	go(target)
	{
		window.open(this.directory+target);
	}
};
class Remove
{
	constructor(root)
	{
		this.root = root;
		this.uploadXHR = new XMLHttpRequest();
		this.isready = false;
		this.onload = undefined;
		this.onauthenticate = undefined;
		this.onnotfound = undefined;
	}

	open(directory)
	{
		this.directory = this.root;
		this.directory += directory
		if (directory[directory.length - 1] != '/')
			this.directory += '/';
	}

	set(file)
	{
		this.file = file;
	}

	exec(authorization)
	{
		const self = this;
		const xhr = self.uploadXHR;

		xhr.onreadystatechange = function()
		{
			if (xhr.readyState === XMLHttpRequest.DONE)
			{
				if (xhr.status === 200)
				{
					self.isready = false;
					if (self.onload != undefined)
					{
						self.file.data = JSON.parse(xhr.responseText);
						self.onload.call(self, self.file);
					}
				}
				else if (xhr.status === 403)
				{
					if (self.onauthenticate != undefined)
						self.onauthenticate.call(self, xhr.getResponseHeader("WWW-Authenticate"), "logout");
				}
				else if (xhr.status === 404)
				{
					if (self.onnotfound != undefined)
						self.onnotfound.call(self, xhr.responseURL);
				}
				else if (self.onerror != undefined)
				{
					self.onerror.call(self, xhr.status);
				}
			}
			return true;
		}
		var directory = self.directory;
		if (self.directory == undefined)
			directory = "";
		xhr.open("DELETE", directory+self.file.name);
		xhr.responseType = "text/json";
		xhr.setRequestHeader("X-Requested-With", "XMLHttpRequest");
		if (authorization)
			xhr.setRequestHeader("Authorization",authorization);
		xhr.send();
	}
};
class Change
{
	constructor(root)
	{
		this.root = root;
		this.uploadXHR = new XMLHttpRequest();
		this.isready = false;
		this.onload = undefined;
		this.onauthenticate = undefined;
		this.onnotfound = undefined;
		this.post = new Object();
	}

	open(directory)
	{
		this.directory = this.root;
		this.directory += directory
		if (directory[directory.length - 1] != '/')
			this.directory += '/';
	}

	set(file)
	{
		this.file = file;
	}

	post(message, type)
	{
		this.post.type = type;
		if (this.post.type == "text/json")
			this.post.data = JSON.stringify(message);
		else
			this.post.data = message;
	}

	exec(newname, authorization)
	{
		const self = this;
		const xhr = self.uploadXHR;

		xhr.onreadystatechange = function()
		{
			if (xhr.readyState === XMLHttpRequest.DONE)
			{
				if (xhr.status === 200)
				{
					self.isready = false;
					if (self.onload != undefined)
					{
						var type = xhr.getResponseHeader("Content-Type");
						if (type == "text/json")
							self.file.data = JSON.parse(xhr.response);
						else
							self.file.data = xhr.response;
						self.onload.call(self, file);
					}
				}
				else if (xhr.status === 403)
				{
					if (self.onauthenticate != undefined)
						self.onauthenticate.call(self, xhr.getResponseHeader("WWW-Authenticate"), "logout");
				}
				else if (xhr.status === 404)
				{
					if (self.onnotfound != undefined)
						self.onnotfound.call(self, xhr.responseURL);
				}
				else if (self.onerror != undefined)
				{
					self.onerror.call(self, xhr.status);
				}
			}
			return true;
		}
		var directory = self.directory;
		if (self.directory == undefined)
			directory = "";
		xhr.open("POST", directory+self.file.name);
		xhr.setRequestHeader("X-Requested-With", "XMLHttpRequest");
		xhr.setRequestHeader("Content-Type", self.post.type);
		if (authorization)
			xhr.setRequestHeader("Authorization",authorization);
		xhr.send(self.post.data);
	}
};
class UpLoader
{
	constructor(root)
	{
		this.root = root;
		this.uploadXHR = new XMLHttpRequest();
		this.reader = new FileReader();
		this.file = undefined;
		this.directory = undefined;
		this.isready = false;
		this.onload = undefined;
		this.onupload = undefined;
		this.onauthenticate = undefined;
		this.onnotfound = undefined;
	}

	open(directory)
	{
		this.directory = this.root;
		this.directory += directory
		if (directory[directory.length - 1] != '/')
			this.directory += '/';
		//this.file.data = "";
	}
	set(file)
	{
		this.file = file;
	}
	get(file)
	{
		const self = this;
		
		self.file = file;
		self.reader.onloadend = function(evt)
			{
				const array = new Uint8ClampedArray(evt.target.result);
				self.file.data = array;

				if (self.reader.readyState == 2)
				{
					self.isready = true;
					if (self.onload != undefined)
						self.onload.call(self, self.file);
				}
			};
		self.reader.onerror = function(err) {
          alert("load error "+err);
        };
		self.reader.readAsArrayBuffer(self.file);
	}
	exec(authorization)
	{
		const self = this;
		const xhr = self.uploadXHR;

		xhr.onreadystatechange = function()
		{
			if (xhr.readyState === XMLHttpRequest.DONE)
			{
				if (xhr.status === 200)
				{
					self.isready = false;
					if (self.onupload != undefined)
						self.onupload.call(self, JSON.parse(xhr.responseText));
					self.file = undefined;
				}
				else if (xhr.status === 403 || xhr.status === 401)
				{
					if (self.onauthenticate != undefined)
						self.onauthenticate.call(self, xhr.getResponseHeader("WWW-Authenticate"), "logout");
				}
				else if (xhr.status === 404)
				{
					if (self.onnotfound != undefined)
					{
						var url = document.createElement('a');
						url .href = xhr.responseURL;
						self.onnotfound.call(self, url);
					}
				}
				else if (self.onerror != undefined)
				{
					self.onerror.call(self, xhr.status);
				}
			}
			return true;
		}
		xhr.ontimeout = function()
		{
			alert("Uploader timeout");
		}
		var data = undefined;
		if (self.file != undefined)
		{
			var filename = self.directory+self.file.name;
			xhr.open("PUT", filename);
			data = self.file;
		}
		else
		{
			xhr.open("PUT", self.directory);
		}
		xhr.responseType = "text/json";
		xhr.setRequestHeader("X-Requested-With", "XMLHttpRequest");
		if (authorization)
			xhr.setRequestHeader("Authorization",authorization);
		xhr.send(data);
	}
};
class Shell
{
	constructor(output)
	{
		var root = location.search.substring(1).split("&");
		root = root.find(function(elem){
				return elem.startsWith("root=");
			});
		if (root)
		{
			root = root.split("=")[1];
			if (root.lastIndexOf('/') != root.length - 1)
				root += '/';
			this.root = root.replace(/\\/g,'/').replace(/\/[^\/]*$/, '');
		}
		else
			this.root = location.pathname.replace(/\\/g,'/').replace(/\/[^\/]*$/, '');
		this.cwd = "/";
		this.dashboard = new Array();
		const self = this;
		this.open = new Open(this.root);
		this.open.onauthenticate = function(challenge, result)
		{
			self.authenticate.challenge = challenge;
			if (self.onauthenticate != undefined)
				self.onauthenticate.call(self, challenge, result);
		}
		this.open.onnotfound = function(file)
		{
			if (self.onnotfound != undefined)
				self.onnotfound.call(self, file);
		}
		this.open.onerror = function(status)
		{
			if (self.onerror != undefined)
				self.onerror(status);
		}
		this.remove = new Remove(this.root);
		this.remove.onload = function(file)
		{
			self.ls();
		}
		this.remove.onauthenticate = function(challenge, result)
		{
			self.authenticate.challenge = challenge;
			if (self.onauthenticate != undefined)
				self.onauthenticate.call(self, challenge, result);
		}
		this.remove.onerror = function(status)
		{
			if (self.onerror != undefined)
				self.onerror(status);
		}
		this.uploader = new UpLoader(this.root);
		this.onput = undefined;
		this.uploader.onload = function(file)
		{
			var ret = true;
			if (self.onput != undefined)
				ret = self.onput.call(self, file);
			if (self.uploader.isready && (ret == true || ret == undefined))
				self.uploader.exec(self.authorization);
		}
		this.uploader.onauthenticate = function(challenge, result)
		{
			self.authenticate.challenge = challenge;
			if (self.onauthenticate != undefined)
				self.onauthenticate.call(self, challenge, result);
		}
		this.uploader.onerror = function(status)
		{
			if (self.onerror != undefined)
				self.onerror(status);
		}
		this.authenticate = new Authenticate("Basic");
		this.authenticate.onauthorization = function(user)
		{
			self.user = user;
			if (self.onauthorization != undefined)
				self.onauthorization.call(self, user);
			self.authorization = self.authenticate.authorization;
		}
		this.authenticate.onauthenticate = function(challenge, result)
		{
			self.authenticate.challenge = challenge;
			if (self.onauthenticate != undefined)
				self.onauthenticate.call(self, challenge, result);
		}
		this.authenticate.onerror = function(status)
		{
			if (self.onerror != undefined)
				self.onerror(status);
		}
		this.authenticate.get();
	}
	generateid()
	{
		return 1;
	}
	login(user, password)
	{
		if (user != undefined)
		{
			this.authenticate.username = user;
			this.authenticate.password = password;
			this.authenticate.build();
		}
		this.authenticate.get();
	}
	logout()
	{
		this.authenticate.destroy();
		this.authenticate.get();
	}
	cd(directory)
	{
		const self = this;
		const id = self.generateid();
		if (self.onbegin != undefined)
		{
			self.onbegin(id);
		}
		self.cwd = directory;
		if (self.oncompleted != undefined)
		{
			self.oncompleted(id);
		}
	}
	ls(directory)
	{
		var filename = undefined;
		if (directory == undefined)
		{
			directory = this.cwd;
		}
		else if (directory[0] != '/')
		{
			directory = this.cwd + directory;
		}
		const self = this;
		const id = this.generateid();
		this.open.onload = function(resultjson)
		{
			self.content = resultjson.content;
			if (self.onchange != undefined)
			{
				var i = self.content.length - 1;
				if (self.content[i].name == undefined)
				{
					self.content.splice(i, 1);
				}
				self.onchange(self.content);
			}
		};
		this.open.open(directory);
		var file = new Blob();
		file.name = filename;
		this.open.set(file);
		this.open.exec(this.authorization);
		return id;
	}
	launch(file)
	{
		this.open.open(this.cwd);
		this.open.go(file);
	}
	rm(filename)
	{
		const self = this;
		const id = this.generateid();
		if (self.onbegin != undefined)
		{
			self.onbegin(id);
		}
		self.remove.onload = function(file)
		{
			if (self.oncompleted != undefined)
			{
				self.oncompleted(id);
			}
		};
		var file = new Blob();
		file.name = filename;
		self.remove.open(self.cwd);
		self.remove.set(file);
		self.remove.exec(self.authorization);
		return id;
	}
	paste()
	{
		const self = this;
		const id = this.generateid();
		if (self.onbegin != undefined)
		{
			self.onbegin(id);
		}
		if (self.dashboard.length > 0)
		{
			var copy = self.dashboard.shift();
			if (copy.type != undefined)
			{
				self.uploader.onupload = function(file)
				{
					if (file.cut == true)
					{
						self.remove.onload = function(file)
						{
							if (self.oncompleted != undefined)
							{
								self.oncompleted(id);
							}
						};
						self.remove.open(self.cwd);
						file.name = file.oldname;
						self.remove.set(file);
						self.remove.exec(self.authorization);
					}
					else
					{
						if (self.oncompleted != undefined)
						{
							self.oncompleted(id);
						}
					}
				};
				self.uploader.open(self.cwd);
				self.uploader.set(copy.file);
				self.uploader.exec(self.authorization);
			}
		}
		return id;
	}
	cp(filename, copyname, cut)
	{
		var file = new Blob();
		file.name = filename;
		file.newname = copyname;
		file.cut = cut;
		const self = this;
		const id = this.generateid();
		if (self.onbegin != undefined)
		{
			self.onbegin(id);
		}
		this.open.onload = function(file)
		{
			if (file.newname != undefined)
			{
				self.uploader.onupload = function(file)
				{
					if (self.oncompleted != undefined)
					{
						self.oncompleted(id);
					}
				};
				self.uploader.open(self.cwd);
				file.name = file.newname;
				self.uploader.set(file);
				self.uploader.exec(self.authorization);
			}
			else
			{
				self.dashboard.unshift(file);
			}
		}
		this.open.open(this.cwd);
		this.open.set(file);
		this.open.exec(this.authorization);
		return id;
	}
	mv(oldname, newname)
	{
		var fileold = new Blob();
		fileold.name = oldname;
		const self = this;
		const id = this.generateid();
		if (self.onbegin != undefined)
		{
			self.onbegin(id);
		}
		this.open.onload = function(file)
		{
			//consol.log("file "+file.name+" loaded");
			self.uploader.onupload = function(resultjson)
			{
				//consol.log("file "+file.name+" uploaded");
				self.remove.onload = function(file)
				{
					if (self.oncompleted != undefined)
					{
						self.oncompleted(id);
					}
				};
				self.remove.open(self.cwd);
				self.remove.set(fileold);
				self.remove.exec(self.authorization);
			}
			self.uploader.open(self.cwd);
			file.name = newname;
			self.uploader.set(file);
			self.uploader.exec(self.authorization);
		}
		this.open.open(this.cwd);
		this.open.set(fileold);
		this.open.exec(this.authorization);
		return id;
	}
	mkdir(directory)
	{
		const self = this;
		const id = this.generateid();
		if (self.onbegin != undefined)
		{
			self.onbegin(id);
		}
		this.uploader.onupload = function(resultjson)
		{
			if (self.oncompleted != undefined)
			{
				self.oncompleted(id);
			}
		};
		if (directory != undefined)
			this.uploader.open(this.cwd+"/"+directory);
		else
			this.uploader.open(this.cwd);
		this.uploader.set();
		this.uploader.exec(this.authorization);
		return id;
	}
	put(file)
	{
		const self = this;
		const id = this.generateid();
		if (self.onbegin != undefined)
		{
			self.onbegin(id);
		}
		this.uploader.onupload = function(resultjson)
		{
			if (self.oncompleted != undefined)
			{
				self.oncompleted(id);
			}
		};
		this.uploader.open(this.cwd);
		this.uploader.get(file);
		return id;
	}
};
