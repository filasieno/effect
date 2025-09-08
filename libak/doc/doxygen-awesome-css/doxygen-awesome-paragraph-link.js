(function(){
  document.addEventListener('DOMContentLoaded', function(){
    var headers = document.querySelectorAll('h1, h2, h3, h4, h5, h6');
    headers.forEach(function(h){
      if(!h.id) return;
      var a = document.createElement('a');
      a.href = '#' + h.id;
      a.textContent = '¶';
      a.className = 'para-link';
      a.style.marginLeft = '0.5em';
      h.appendChild(a);
    });
  });
})();
