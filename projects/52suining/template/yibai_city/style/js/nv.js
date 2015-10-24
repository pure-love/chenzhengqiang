	var FixedBox=function(el){
		this.element=el;
		this.BoxY=getXY(this.element).y;
	}
	FixedBox.prototype={
		setCss:function(){
			var windowST=(document.compatMode && document.compatMode!="CSS1Compat")? document.body.scrollTop:document.documentElement.scrollTop||window.pageYOffset;
			var div = document.getElementById('aqx_nav');
			if(windowST>this.BoxY){
				this.element.style.cssText="position:fixed;top:0px;width:100%;z-index:180;background:#fff;left:0px;padding:0px 0px;margin-top:0; opacity:1; -moz-opacity:1; filter:alpha(opacity=100);height: 45px;overflow: hidden;border-bottom: 1px solid #D5D5D5;";
				div.className = 'aqx_navc';
			}else{
				this.element.style.cssText="";
				 div.className ='';
			}
		}
	};

	function addEvent(elm, evType, fn, useCapture) {
		if (elm.addEventListener) {
			elm.addEventListener(evType, fn, useCapture);
		return true;
		}else if (elm.attachEvent) {
			var r = elm.attachEvent('on' + evType, fn);
			return r;
		}
		else {
			elm['on' + evType] = fn;
		}
	}

	function getXY(el) {
        return document.documentElement.getBoundingClientRect && (function() {
            var pos = el.getBoundingClientRect();
            return { x: pos.left + document.documentElement.scrollLeft, y: pos.top + document.documentElement.scrollTop };
        })() || (function() {
            var _x = 0, _y = 0;
            do {
                _x += el.offsetLeft;
                _y += el.offsetTop;
            } while (el = el.offsetParent);
            return { x: _x, y: _y };
        })();
    }

	var divA=new FixedBox(document.getElementById("aqx_nav"));
   	addEvent(window,"scroll",function(){
		divA.setCss();
	});
	