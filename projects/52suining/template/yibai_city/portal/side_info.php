<?php echo 'www.aiyudi.com';exit;?>
<style>
.viewSide_info{display:none;position:fixed;left: 50%; top:20%;margin-left: -608px;width:48px;overflow:hidden;border:1px solid #ececec;z-index:1001;-webkit-border-radius:3px 0 0 3px;border-radius:3px 0 0 3px;}
.viewSide_info .like,.viewSide_info .comment,.viewSide_info .collect,.viewSide_info .recommend,.viewSide_info .share{padding:6px 0;display:block;height:36px;border-bottom:1px solid #e9e9e9;border-top:1px solid #fff;padding-left:48px;width:45px;background-color:#f5f5f5;background-image:url($_G['style'][styleimgdir]/viewSide_info.png);background-repeat:no-repeat;}
.viewSide_info span{display:block;color:#888; text-align:center;}
.viewSide_info .like{border-top-width:0;-webkit-border-radius:3px 0 0 0;border-radius:3px 0 0 0;background-position:7px 10px;}
.viewSide_info .recommend{background-position:7px -377px;}
.viewSide_info .comment{background-position:7px -53px;}
.viewSide_info .collect{background-position:7px -507px;}
.viewSide_info .share{background-position:7px -126px;}

.viewSide_info.hover{width:93px;border:none; margin-left:-621px;}
.viewSide_info.hover a:hover{text-decoration:none;}
.viewSide_info.hover a:hover span{color:#225588;}
.viewSide_info.hover .like:hover{background-position:7px -194px;}
.viewSide_info.hover .recommend:hover{background-position:7px -442px;}
.viewSide_info.hover .comment:hover{background-position:7px -250px;}
.viewSide_info.hover .collect:hover{background-position:7px -572px;}
.viewSide_info.hover .share:hover{background-position:7px -313px;}

/* BAIDU */
.bdshare-button-style0-16 a, .bdshare-button-style0-16 .bds_more{ background-position:1000px 1000px;}
.bdshare-button-style0-16 a, .bdshare-button-style0-16 .bds_more{ padding-left:0!important; margin:0!important; width:45px!important; height:48px; display:block;}
.bdshare_popup_box{ margin-left:-25px; margin-top:15px;}
</style>
    
    
    
<div class="viewSide_info">
    <!--{template home/click_num}--> 
    <a class="comment" href="portal.php?mod=view&aid=$article[aid]#comment">
        <span>评论</span>
        <span>$article[commentnum]</span>
    </a>    
    <a class="collect" href="home.php?mod=spacecp&ac=favorite&type=article&id=$article[aid]&handlekey=favoritearticlehk_{$article[aid]}" id="a_favorite" onclick="showWindow(this.id, this.href, 'get', 0);">
        <span>{lang favorite}</span>
        <span>$article[favtimes]</span>
    </a>
    <!--{if helper_access::check_module('share')}--> 
    <a href="home.php?mod=spacecp&ac=share&type=article&id=$article[aid]&handlekey=sharearticlehk_{$article[aid]}" id="a_share" onclick="showWindow(this.id, this.href, 'get', 0);" class="share">
        <span>{lang share}</span>
        <span>$article[sharetimes]</span>
    </a>
    <!--{/if}--> 
    <a href="javascript:void(0)" class="recommend bds_more bdsharebuttonbox" data-cmd="more">
        <span>分享</span>
        <span>此文</span>
    </a>
    <script>window._bd_share_config={"common":{"bdSnsKey":{},"bdText":"","bdMini":"1","bdMiniList":false,"bdPic":"","bdStyle":"0","bdSize":"16"},"share":{}};with(document)0[(getElementsByTagName('head')[0]||body).appendChild(createElement('script')).src='http://bdimg.share.baidu.com/static/api/js/share.js?v=89860593.js?cdnversion='+~(-new Date()/36e5)];</script>
</div>
    
<script type="text/javascript">
(function() {
    jQuery(window).scroll(function() {
        if (jQuery(window).scrollTop() > 100) {
            jQuery('.viewSide_info').fadeIn();
        } else if (jQuery(window).scrollTop() < 100) {
            jQuery('.viewSide_info').fadeOut();
        }
    });
    jQuery(".viewSide_info").hover(function() {
        jQuery(this).addClass("hover");
    },
    function() {
        jQuery(this).removeClass("hover");
    })

})();


</script>

