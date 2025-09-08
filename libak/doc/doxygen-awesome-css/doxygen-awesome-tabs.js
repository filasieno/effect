(function(){
  document.addEventListener('DOMContentLoaded', function(){
    document.querySelectorAll('[data-tabs] .tab').forEach(function(tab){
      tab.addEventListener('click', function(){
        var wrap = tab.closest('[data-tabs]');
        if(!wrap) return;
        wrap.querySelectorAll('.tab').forEach(function(t){t.classList.remove('active');});
        tab.classList.add('active');
        var target = wrap.querySelector(tab.getAttribute('data-target'));
        wrap.querySelectorAll('.tab-panel').forEach(function(p){p.hidden = true;});
        if(target) target.hidden = false;
      });
    });
  });
})();
