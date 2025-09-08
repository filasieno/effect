(function(){
  try {
    var key = 'doxygen-awesome-darkmode';
    var pref = localStorage.getItem(key) || 'auto';
    function apply(mode){
      document.documentElement.dataset.theme = mode;
    }
    function current(){
      if(pref === 'auto') return (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) ? 'dark' : 'light';
      return pref;
    }
    window.toggleDoxygenAwesomeDarkMode = function(){
      pref = current() === 'dark' ? 'light' : 'dark';
      localStorage.setItem(key, pref);
      apply(pref);
    };
    apply(current());
  } catch(e) {}
})();
