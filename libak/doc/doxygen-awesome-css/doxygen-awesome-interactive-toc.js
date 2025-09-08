(function(){
  document.addEventListener('DOMContentLoaded', function(){
    var toc = document.getElementById('nav-tree-contents') || document.getElementById('nav-tree');
    if(!toc) return;
    toc.addEventListener('click', function(e){
      var t = e.target;
      if(t && t.classList && t.classList.contains('arrow')){
        var li = t.closest('li');
        if(li) li.classList.toggle('open');
      }
    });
  });
})();
