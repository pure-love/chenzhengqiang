<?php if(!defined('IN_DISCUZ')) exit('Access Denied'); hookscriptoutput('index');?><?php include template('common/header'); ?><style id="diy_style" type="text/css"></style>
<script src="<?php echo $_G['style']['styleimgdir'];?>/js/jquery.hoverdir.js" type="text/javascript"></script>
<div class="wp">
<!--[diy=diy1]--><div id="diy1" class="area"></div><!--[/diy]-->
</div>
<!--正文内容一-->
<div class="yb_mainone">

  <!--left-->
  <div class="yb_moleft">
      
    <!--轮播--><div class="yb_lb"></div><!--轮播end-->
     <div id="yb_lunbo" class="slidebox">
       <!--轮播-->  
    <!--[diy=diycity801]--><div id="diycity801" class="area"></div><!--[/diy]-->
    </div><!--轮播end-->
      
    <!--资讯导读-->
    <div class="yb_smmain">
       <div class="yb_smmtop"><span>资讯导读</span></div>
       	<!--[diy=diycity201]--><div id="diycity201" class="area"></div><!--[/diy]-->
 
        
    </div>
    <!--资讯导读-end-->    
  </div>
  <!--leftend-->

  <!--头条报道-->
  <div class="yb_mocenter">  
  
    <div class="ybmoc_top"><span style="float:right;"><iframe width="180" height="36" frameborder="0" src="http://tianqi.2345.com/plugin/widget/index.htm?s=3&amp;z=3&amp;t=1&amp;v=0&amp;d=3&amp;bd=0&amp;k=000000&amp;f=&amp;q=1&amp;e=1&amp;a=1&amp;c=54511&amp;w=180&amp;h=36&amp;align=right" scrolling="no" allowtransparency="true"></iframe></span></div>
    	<!--[diy=diycity202]--><div id="diycity202" class="area"></div><!--[/diy]-->
  		
   
    
  </div>
  <!--头条报道end-->


  <!--right-->
  <div class="yb_moright">
       <div class="yb_login">
  <?php if($_G['uid']) { ?>
      <?php $kmmember = C::t('common_member_count')->fetch($_G['uid']);?>     <!--登录-->
     <!--登录后-->
      

          <div class="mthrb_lone">
         <div class="rhy_tqhyr"><a href="home.php?mod=space&amp;uid=<?php echo $_G['uid'];?>"><?php echo avatar($_G[uid],middle);?></a></div>
         <div class="rhy_tqhyl2">
           <p><a href="home.php?mod=space&amp;uid=<?php echo $_G['uid'];?>"><strong><?php echo $_G['member']['username'];?></strong></a>，你好！</p>           
           <div class="rhy_tqhyl2_div">
            <a href="home.php?mod=space&amp;do=friend"><strong><?php echo $kmmember['friends'];?></strong>好友</a>
             <a href="forum.php?mod=guide&amp;view=my"><strong><?php echo $kmmember['posts'];?></strong>帖子</a>
             <a href="home.php?mod=spacecp&amp;ac=credit&amp;showcredit=1"><strong><?php echo $_G['member']['credits'];?></strong>积分</a>
           </div>    
         </div>
           </div> 
         <div class="mthrb_ltwoc">
         <a href="home.php?mod=space&amp;uid=<?php echo $_G['uid'];?>&amp;do=profile" onClick="showWindow('login', this.href)" class="twoa">会员中心</a>       
         <a href="home.php?mod=spacecp">资料修改</a>
         <a  href="forum.php?mod=misc&amp;action=nav" onclick="showWindow('nav', this.href, 'get', 0)">快速发布</a>
       </div>  

      
 
     <!--登录后end-->
      <?php } else { ?>
  
         <div class="mthrb_lone">
         <dl>
           <dt><a href=""><img src="template/yibai_city2/style/aqx_img1.png" /></a></dt>
           <dd><a href="" class="o_a1">HI，您好！ </a><bR />欢迎您来到艺佰设计领域网站</dd>
         </dl>
       </div> 
       <div class="mthrb_ltwo">
         <a href="member.php?mod=logging&amp;action=login" onClick="showWindow('login', this.href)" class="twoa">会员登录</a>       
         <a href="member.php?mod=<?php echo $_G['setting']['regname'];?>">马上注册</a>
         <a  href="forum.php?mod=misc&amp;action=nav" onclick="showWindow('nav', this.href, 'get', 0)">快速发布</a>
       </div>  
       
        <!--登录end-->
       <?php } ?>   
       <div class="mthrb_lthr">
        <!--[diy=diycity203]--><div id="diycity203" class="area"></div><!--[/diy]-->
      
      
      
       </div>
    </div>
    <script type="text/javascript">
jQuery(".mthrb_lthr").slide({mainCell:".bd ul",autoPage:true,effect:"topLoop",autoPlay:true,vis:1});
</script>
    <!--网站导航-->
    <div class="yb_smmain">
       <div class="yb_smmtop"><span>网站导航</span></div>
       
     <!--[diy=diycity204]--><div id="diycity204" class="area"></div><!--[/diy]-->  
       
        
    </div>
    <!--网站导航-end--> 

    <!--最新活动-->
    <div class="yb_smmain">
       <div class="yb_smmtop"><span>最新活动</span></div>
       <!--[diy=diycity205]--><div id="diycity205" class="area"></div><!--[/diy]-->
      
    </div>
    <!--最新活动-end-->    
  </div>
  <!--rightend-->

</div>
<!--正文内容一end-->

<!--banner--><div class="aqx_banner">
    <!--[diy=diycity206]--><div id="diycity206" class="area"></div><!--[/diy]-->
</div><!--banner-->
<script type="text/javascript">
jQuery(document).ready(function(){jQuery(function() {jQuery('.aqx_tpbot .list').hoverdir();});});
</script>
<!--贴图专区-->
<div class="aqx_teipic">
  <div class="aqx_tptop">
    <div class="aqx_tptl"><span>贴图</span>专区</div>
    <div class="aqx_tptr"><a href="#">精选图片</a><span>/</span><a href="#">社会实时</a><span>/</span><a href="#">鲜人鲜事</a></div>
  </div>
  <div class="aqx_tpbot">

    <div class="aqx_tpbm">
        <!--[diy=diycity207]--><div id="diycity207" class="area"></div><!--[/diy]-->
   
    </div>
    
    <div class="aqx_tpbm">    
      <div class="tpbm3 list">
          <!--[diy=diycity208]--><div id="diycity208" class="area"></div><!--[/diy]-->
     </div>
      <div class="aqx_tpbm2">
        <div class="tpbm4">
            <!--[diy=diycity209]--><div id="diycity209" class="area"></div><!--[/diy]-->

        </div>
      <div class="tpbm4 list">
          <!--[diy=diycity2010]--><div id="diycity2010" class="area"></div><!--[/diy]-->
      
      </div>
    </div>    
    </div>
    
    <div class="aqx_tpbm">    
      <div class="aqx_tpbm2">        
      <div class="tpbm4 list">
         <!--[diy=diycity2011]--><div id="diycity2011" class="area"></div><!--[/diy]-->
      </div>
      <div class="tpbm4">
         <!--[diy=diycity2012]--><div id="diycity2012" class="area"></div><!--[/diy]-->
    
        </div>
    </div>  
      <div class="tpbm2 list">
         <!--[diy=diycity2013]--><div id="diycity2013" class="area"></div><!--[/diy]-->
      </div>
      <div class="tpbm2 list">
         <!--[diy=diycity2014]--><div id="diycity2014" class="area"></div><!--[/diy]-->
      </div>
    </div>

  </div>
</div>
<!--贴图专区end-->

<!--banner--><div class="aqx_banner">
 <!--[diy=diyyibaiad02]--><div id="diyyibaiad02" class="area"></div><!--[/diy]-->
</div><!--banner-->


<!--城市速递-->
<div class="aqx_city">
  <div class="aqx_citytop">
    <div class="aqx_citl"><span>城市</span>速递</div>
    <div class="aqx_citr"><a href="#">精选图片</a><span>/</span><a href="#">社会实时</a><span>/</span><a href="#">鲜人鲜事</a></div>
  </div>
  <div class="aqx_citybot">
    
    <!--left-->
    <div class="cib_left">
    
      <div class="cib_left_div">
        <div class="left_divtop">
            <div class="left_dtl"><span class="span1">城市资讯</span></div>
            <div class="left_dtr"><a href="#">自定内容</a><span>/</span><a href="#">自定内容</a></div>
        </div>
         <!--[diy=diyyibaicity2c01]--><div id="diyyibaicity2c01" class="area"></div><!--[/diy]-->
       
      </div>
      
      <div class="cib_left_div">
        <div class="left_divtop">
            <div class="left_dtl"><span class="span2">吃喝玩乐</span></div>
            <div class="left_dtr"><a href="#">自定内容</a><span>/</span><a href="#">自定内容</a></div>
        </div>
       <!--[diy=diyyibaicity2c02]--><div id="diyyibaicity2c02" class="area"></div><!--[/diy]-->
      </div>
      
      <div class="cib_left_div">
        <div class="left_divtop">
            <div class="left_dtl"><span class="span3">休闲娱乐</span></div>
            <div class="left_dtr"><a href="#">自定内容</a><span>/</span><a href="#">自定内容</a></div>
        </div>
        <!--[diy=diyyibaicity2c03]--><div id="diyyibaicity2c03" class="area"></div><!--[/diy]-->
      </div>
      
      <div class="cib_left_div">
        <div class="left_divtop">
            <div class="left_dtl"><span class="span4">点滴生活</span></div>
            <div class="left_dtr"><a href="#">自定内容</a><span>/</span><a href="#">自定内容</a></div>
        </div>
        <!--[diy=diyyibaicity2c04]--><div id="diyyibaicity2c04" class="area"></div><!--[/diy]-->
      </div>
      
      <div class="cib_left_div">
        <div class="left_divtop">
            <div class="left_dtl"><span class="span5">我要结婚</span></div>
            <div class="left_dtr"><a href="#">自定内容</a><span>/</span><a href="#">自定内容</a></div>
        </div>
        <!--[diy=diyyibaicity2c05]--><div id="diyyibaicity2c05" class="area"></div><!--[/diy]-->
      </div>
      
      <div class="cib_left_div">
        <div class="left_divtop">
            <div class="left_dtl"><span class="span6">亲子乐园</span></div>
            <div class="left_dtr"><a href="#">自定内容</a><span>/</span><a href="#">自定内容</a></div>
        </div>
        <!--[diy=diyyibaicity2c06]--><div id="diyyibaicity2c06" class="area"></div><!--[/diy]-->
      </div>
      
      <div class="cib_left_div">
        <div class="left_divtop">
            <div class="left_dtl"><span class="span7">房产家居</span></div>
            <div class="left_dtr"><a href="#">自定内容</a><span>/</span><a href="#">自定内容</a></div>
        </div>
        <!--[diy=diyyibaicity2c07]--><div id="diyyibaicity2c07" class="area"></div><!--[/diy]-->
      </div>
      
      <div class="cib_left_div">
        <div class="left_divtop">
            <div class="left_dtl"><span class="span8">汽车保养</span></div>
            <div class="left_dtr"><a href="#">自定内容</a><span>/</span><a href="#">自定内容</a></div>
        </div>
        <!--[diy=diyyibaicity2c08]--><div id="diyyibaicity2c08" class="area"></div><!--[/diy]-->
      </div>
    
    </div>
    <!--leftend-->
    <!--right-->
    <div class="cib_right">
    
       <!--网友热议-->
       <div class="cibr_friend">       
         <div class="cibr_fritop"><span>网友热议</span></div>
         <div class="cibr_fribox">
         <div class="cibr_fribot">
           <!--[diy=diyyibaiwangyou01]--><div id="diyyibaiwangyou01" class="area"></div><!--[/diy]-->
         </div>
         </div>
         
       </div>
       <!--网友热议end-->
       
       <!--商家推荐-->
       <div class="cibr_friend">
         <div class="cibr_fritop"><span>商家推荐</span></div>
         <div class="cibr_groom">
          <!--[diy=diyyibaisahng01]--><div id="diyyibaisahng01" class="area"></div><!--[/diy]-->
         
         
         </div>
       </div>
       <!--商家推荐end-->       
       
    </div>
    <!--rightends-->

  </div>
</div>
<!--城市速递end-->

<!--banner--><div class="aqx_banner">
 <!--[diy=diyyibaiad01]--><div id="diyyibaiad01" class="area"></div><!--[/diy]-->
</div><!--banner-->

<!--惠选生活-->
<div class="aqx_life">
  <div class="aqx_lifetop">
    <div class="aqx_lifetl"><span>惠选</span>生活</div>
    <div class="aqx_lifetr"><a href="#">精选图片</a><span>/</span><a href="#">社会实时</a><span>/</span><a href="#">鲜人鲜事</a></div>
  </div>
   <!--[diy=diyyibaiad041501]--><div id="diyyibaiad041501" class="area"></div><!--[/diy]-->
  
  
</div>
<!--惠选生活end-->


<!--便民信息-->
<div class="aqx_people">
  <div class="aqx_peotop">
    <div class="aqx_peotl"><span>便民</span>信息</div>
        <div class="hd">
    <div class="aqx_peotr">

    <ul><li><a href="javascript:;">租房</a></li><li><a href="javascript:;">二手</a></li><li><a href="javascript:;">求职</a></li><li><a href="javascript:;">招聘</a></li></ul>
    </div>
    </div>
</div>
    <div class="bd">
  <div class="aqx_peobot">
  		<div class="fiic_tt">
     <!--[diy=diyyibaiad041502]--><div id="diyyibaiad041502" class="area"></div><!--[/diy]-->
</div>
    <div class="fiic_pic">
         <!--[diy=diyyibaiad041503]--><div id="diyyibaiad041503" class="area"></div><!--[/diy]-->
   
    </div>  

</div> 

<!--111111111-->
      <div class="aqx_peobot">
      <div class="fiic_tt">
           <!--[diy=diyyibaiad041504]--><div id="diyyibaiad041504" class="area"></div><!--[/diy]-->
    </div>
    
    <div class="fiic_pic">
         <!--[diy=diyyibaiad041505]--><div id="diyyibaiad041505" class="area"></div><!--[/diy]-->
    
    </div>  

       </div> 
       
       <!--222222222-->
         <div class="aqx_peobot">
          <div class="fiic_tt">
              <!--[diy=diyyibaiad041506]--><div id="diyyibaiad041506" class="area"></div><!--[/diy]-->
    	</div>
    <div class="fiic_pic">
         <!--[diy=diyyibaiad041507]--><div id="diyyibaiad041507" class="area"></div><!--[/diy]-->
      
    </div>  

       </div> 
       <!--3333333333-->
         <div class="aqx_peobot">
          <div class="fiic_tt">
              <!--[diy=diyyibaiad041508]--><div id="diyyibaiad041508" class="area"></div><!--[/diy]-->
       </div> 
    <div class="fiic_pic">
    <!--[diy=diyyibaiad041509]--><div id="diyyibaiad041509" class="area"></div><!--[/diy]-->
     
    </div>  

       </div> 
              
  </div>
</div>
<!--便民信息end-->

<!--banner--><div class="aqx_banner"><a href="#"><img src="template/yibai_city2/style/aqx_img20.png" /></a></div><!--banner-->

<!--美图视界-->
<div class="aqx_beauty">
  <div class="aqx_beautytop">
    <div class="aqx_beautytl"><span>美图</span>视界</div>
    <div class="aqx_beautytr"><a href="#">精选图片</a><span>/</span><a href="#">社会实时</a><span>/</span><a href="#">鲜人鲜事</a></div>
  </div>
  <div class="aqx_beautybot">     
  
    <div class="yb_hodone">

     <!--[diy=diyyibaicity012801]--><div id="diyyibaicity012801" class="area"></div><!--[/diy]-->
    </div>  
    
    
    <div class="yb_hodthr">

      <div class="yb_hodthrt"><!--[diy=diyyibaicity012802]--><div id="diyyibaicity012802" class="area"></div><!--[/diy]--></div>
      <div class="yb_hodthrt">
      <!--[diy=diyyibaicity012806]--><div id="diyyibaicity012806" class="area"></div><!--[/diy]-->
      
      
      </div>
    </div> 
    
    
    <div class="yb_hodfour">
        <!--[diy=diyyibaicity012803]--><div id="diyyibaicity012803" class="area"></div><!--[/diy]-->

    </div>
   
    
    <div class="yb_hodthr">
              
      <div class="yb_hodthrt">
<!--[diy=diyyibaicity012804]--><div id="diyyibaicity012804" class="area"></div><!--[/diy]-->
    </a>
      </div>
      <div class="yb_hodthrt">
      <!--[diy=diyyibaicity012807]--><div id="diyyibaicity012807" class="area"></div><!--[/diy]-->
      </div>
    </div> 
    
    <div class="yb_hodfour">
              <!--[diy=diyyibaicity012805]--><div id="diyyibaicity012805" class="area"></div><!--[/diy]-->
   
    </div>   
     
     
   
  </div>
</div>
<!--美图视界end-->

<!--友情链接-->
<div class="aqx_links">
  <div class="aqx_linkstop">
    <div class="aqx_linkstl"><span>友情</span>链接</div>
    <div class="aqx_linkstr"></div>
  </div>
  <div class="aqx_linksbot">     
  <!--[diy=diyyibaicity2youqing]--><div id="diyyibaicity2youqing" class="area"></div><!--[/diy]-->
  </div>  
</div>
</div>
<!--友情链接end-->
<script type="text/javascript">
jQuery(".cibr_friend").slide( { mainCell:".cibr_fribot ul",autoPage:true,effect:"topLoop",autoPlay:true,vis:5,easing:"easeOutCirc"});
jQuery(".yb_readbot li").hover(function(){ jQuery(this).addClass("on").siblings().removeClass("on")},function(){ });
jQuery(".aqx_people").slide({trigger:"click",delayTime:1000});
</script><?php include template('common/footer'); ?>